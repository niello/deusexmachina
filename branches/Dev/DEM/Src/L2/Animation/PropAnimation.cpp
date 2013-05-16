#include "PropAnimation.h"

#include <Game/Entity.h>
#include <Game/GameServer.h> // For the time
#include <Scene/PropSceneNode.h>
#include <Scene/SceneServer.h>
#include <Scene/Bone.h>
#include <Animation/KeyframeClip.h>
#include <Animation/MocapClip.h>
#include <Data/DataServer.h>

#include <DB/DBServer.h>

namespace Attr
{
	DefineString(AnimDesc);
}

BEGIN_ATTRS_REGISTRATION(PropAnimation)
	RegisterString(AnimDesc, ReadOnly);
END_ATTRS_REGISTRATION

namespace Anim
{
	bool LoadMocapClipFromNAX2(const nString& FileName, const nDictionary<int, CStrID>& BoneToNode, PMocapClip OutClip);
	bool LoadKeyframeClipFromKFA(const nString& FileName, PKeyframeClip OutClip);
}

namespace Properties
{
__ImplementClass(Properties::CPropAnimation, 'PANM', Game::CProperty);
__ImplementPropertyStorage(CPropAnimation);

using namespace Data;

void CPropAnimation::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	CProperty::GetAttributes(Attrs);
	Attrs.Append(Attr::AnimDesc);
}
//---------------------------------------------------------------------

void CPropAnimation::Activate()
{
	Game::CProperty::Activate();

	PROP_SUBSCRIBE_PEVENT(OnPropsActivated, CPropAnimation, OnPropsActivated);
	PROP_SUBSCRIBE_PEVENT(ExposeSI, CPropAnimation, ExposeSI);
	PROP_SUBSCRIBE_PEVENT(OnBeginFrame, CPropAnimation, OnBeginFrame);
}
//---------------------------------------------------------------------

void CPropAnimation::Deactivate()
{
	UNSUBSCRIBE_EVENT(OnPropsActivated);
	UNSUBSCRIBE_EVENT(ExposeSI);
	UNSUBSCRIBE_EVENT(OnBeginFrame);

	for (int i = 0; i < Tasks.GetCount(); ++i)
		Tasks[i].Stop(0.f);
	Tasks.Clear();

	Clips.Clear();
	Nodes.Clear();

	Game::CProperty::Deactivate();
}
//---------------------------------------------------------------------

bool CPropAnimation::OnPropsActivated(const Events::CEventBase& Event)
{
	CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
	if (!pProp || !pProp->GetNode()) OK; // Nothing to animate

	// Remap bone indices to node relative pathes
	nDictionary<int, CStrID> Bones;
	Bones.Add(-1, CStrID::Empty);
	AddChildrenToMapping(pProp->GetNode(), pProp->GetNode(), Bones);

//!!!to Activate() + (NAX2 loader requires ref-skeleton to remap bone indices to nodes)
	PParams Desc;
	const nString& AnimDesc = GetEntity()->GetAttr<nString>(Attr::AnimDesc);
	if (AnimDesc.IsValid()) Desc = DataSrv->LoadPRM(nString("game:Anim/") + AnimDesc + ".prm");

	if (Desc.IsValid())
	{
		for (int i = 0; i < Desc->GetCount(); ++i)
		{
			CParam& Prm = Desc->Get(i);
			CStrID ClipRsrcID = Prm.GetValue<CStrID>();
			Anim::PAnimClip Clip = SceneSrv->AnimationMgr.GetTypedResource(ClipRsrcID);
			if (!Clip.IsValid())
			{
				nString FileName = ClipRsrcID.CStr();
				if (FileName.CheckExtension("mca") || FileName.CheckExtension("nax2"))
					Clip = n_new(Anim::CMocapClip(ClipRsrcID));
				else if (FileName.CheckExtension("kfa"))
					Clip = n_new(Anim::CKeyframeClip(ClipRsrcID));
				n_verify(SceneSrv->AnimationMgr.AddResource(Clip));
			}
			if (!Clip->IsLoaded())
			{
				nString FileName = Clip->GetUID().CStr();
				if (FileName.CheckExtension("mca") || FileName.CheckExtension("nax2"))
					LoadMocapClipFromNAX2(FileName, Bones, (Anim::CMocapClip*)Clip.GetUnsafe());
				else if (FileName.CheckExtension("kfa"))
					LoadKeyframeClipFromKFA(FileName, (Anim::CKeyframeClip*)Clip.GetUnsafe());
			}
			n_assert(Clip->IsLoaded());
			Clips.Add(Prm.GetName(), Clip);
		}
	}
//!!!to Activate() -

//!!!DBG TMP! some AI character controller must drive character animations
	if (Clips.GetCount())
		StartAnim(CStrID("Walk"), true, 0.f, 1.f, 10, 1.f, 0.f, 0.f);

	OK;
}
//---------------------------------------------------------------------

void CPropAnimation::AddChildrenToMapping(Scene::CSceneNode* pParent, Scene::CSceneNode* pRoot, nDictionary<int, CStrID>& Bones)
{
	for (DWORD i = 0; i < pParent->GetChildCount(); ++i)
	{
		Scene::CSceneNode* pNode = pParent->GetChild(i);
		Scene::CBone* pBone = pNode->FindFirstAttr<Scene::CBone>();
		if (pBone)
		{
			static const nString StrDot(".");
			nString Name = pNode->GetName().CStr();
			Scene::CSceneNode* pCurrParent = pParent;
			while (pCurrParent && pCurrParent != pRoot)
			{
				Name = pCurrParent->GetName().CStr() + StrDot + Name;
				pCurrParent = pCurrParent->GetParent();
			}
			Bones.Add(pBone->GetIndex(), CStrID(Name.CStr()));
			if (!pBone->IsTerminal()) AddChildrenToMapping(pNode, pRoot, Bones);
		}
	}
}
//---------------------------------------------------------------------

bool CPropAnimation::OnBeginFrame(const Events::CEventBase& Event)
{
	float FrameTime = (float)GameSrv->GetFrameTime();
	for (int i = 0; i < Tasks.GetCount(); ++i)
		Tasks[i].Update(FrameTime);
	OK;
}
//---------------------------------------------------------------------

int CPropAnimation::StartAnim(CStrID ClipID, bool Loop, float Offset, float Speed, DWORD Priority,
							  float Weight, float FadeInTime, float FadeOutTime)
{
	if (Speed == 0.f || Weight <= 0.f || Weight > 1.f) return INVALID_INDEX;
	int ClipIdx = Clips.FindIndex(ClipID);
	if (ClipIdx == INVALID_INDEX) return INVALID_INDEX; // Invalid task ID
	Anim::PAnimClip Clip = Clips.ValueAtIndex(ClipIdx);
	if (!Clip->GetSamplerCount() || !Clip->GetDuration()) return INVALID_INDEX;
	if (!Loop && (Offset < 0.f || Offset > Clip->GetDuration())) return INVALID_INDEX;

	CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
	if (!pProp || !pProp->GetNode()) return INVALID_INDEX; // Nothing to animate
	Scene::CSceneNode* pRoot = pProp->GetNode();

	int TaskID = INVALID_INDEX;
	Anim::CAnimTask* pTask = NULL;
	for (int i = 0; i < Tasks.GetCount(); ++i)
		if (!Tasks[i].ClipID.IsValid())
		{
			pTask = &Tasks[i];
			TaskID = i;
			break;
		}

	if (!pTask)
	{
		TaskID = Tasks.GetCount();
		pTask = Tasks.Reserve(1);
	}

	n_assert_dbg(!pTask->Ctlrs.GetCount());

	pTask->Ctlrs.BeginAdd(Clip->GetSamplerCount());
	for (DWORD i = 0; i < Clip->GetSamplerCount(); ++i)
	{
		Scene::CSceneNode* pNode;
		CStrID Target = Clip->GetSamplerTarget(i);
		int NodeIdx = Nodes.FindIndex(Target);
		if (NodeIdx == INVALID_INDEX)
		{
			pNode = pRoot->GetChild(Target.CStr());
			if (pNode) Nodes.Add(Target, pNode);
			else continue;
		}
		else pNode = Nodes.ValueAtIndex(NodeIdx);
		pNode->Controller = Clip->CreateController(i);
		pTask->Ctlrs.Add(pNode, pNode->Controller.GetUnsafe());

		//!!!
		// If still no blend controller, create and setup
		// Add child controller
		// Only blend controller allows to tune weight
		//????or weight to controller?
	}
	pTask->Ctlrs.EndAdd();

	if (!pTask->Ctlrs.GetCount()) return INVALID_INDEX;

	if (!Loop)
	{
		float RealDuration = Clip->GetDuration() / n_fabs(Speed);
		if (FadeInTime + FadeOutTime > RealDuration)
		{
			FadeOutTime = n_max(0.f, RealDuration - FadeInTime);
			FadeInTime = RealDuration - FadeOutTime;
		}
	}

	FadeInTime *= Speed;
	FadeOutTime *= Speed;

	if (!Loop) pTask->StopTimeBase = ((Speed > 0.f) ? Clip->GetDuration() : 0.f) - FadeOutTime;

	pTask->ClipID = ClipID;
	pTask->Clip = Clip;
	pTask->CurrTime = Offset;
	pTask->Offset = Offset;
	pTask->Speed = Speed;
	pTask->Priority = Priority;
	pTask->Weight = Weight;
	pTask->FadeInTime = Offset + FadeInTime;	// Get a point in time becuse we know the start time
	pTask->FadeOutTime = FadeOutTime;			// Remember only the length, because we don't know the end time
	pTask->State = Anim::CAnimTask::Task_Starting;
	pTask->Loop = Loop;

	return TaskID;
}
//---------------------------------------------------------------------

float CPropAnimation::GetAnimLength(CStrID ClipID) const
{
	int ClipIdx = Clips.FindIndex(ClipID);
	if (ClipIdx == INVALID_INDEX) return 0.f;
	return Clips.ValueAtIndex(ClipIdx)->GetDuration();
}
//---------------------------------------------------------------------

} // namespace Properties
