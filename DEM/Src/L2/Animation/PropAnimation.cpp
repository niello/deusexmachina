#include "PropAnimation.h"

#include <Game/Entity.h>
#include <Game/GameServer.h> // For the time
#include <Scene/PropSceneNode.h>
#include <Scene/SceneServer.h>
#include <Scene/Bone.h>
#include <Animation/KeyframeClip.h>
#include <Animation/MocapClip.h>
#include <Animation/AnimControllerKeyframe.h>
#include <Animation/AnimControllerMocap.h>
#include <Data/DataServer.h>

#include <Loading/EntityFactory.h>
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
ImplementRTTI(Properties::CPropAnimation, Game::CProperty);
ImplementFactory(Properties::CPropAnimation);
ImplementPropertyStorage(CPropAnimation, 64);
RegisterProperty(CPropAnimation);

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

	//!!!abort all tasks and destroy controllers!
	Tasks.Clear();

	Clips.Clear();
	Nodes.Clear();

	Game::CProperty::Deactivate();
}
//---------------------------------------------------------------------

bool CPropAnimation::OnPropsActivated(const Events::CEventBase& Event)
{
	CPropSceneNode* pProp = GetEntity()->FindProperty<CPropSceneNode>();
	if (!pProp || !pProp->GetNode()) OK; // Nothing to animate

	// Remap bone indices to node relative pathes
	nDictionary<int, CStrID> Bones;
	Bones.Add(-1, CStrID::Empty);
	AddChildrenToMapping(pProp->GetNode(), pProp->GetNode(), Bones);

//!!!to Activate() + (NAX2 loader requires ref-skeleton to remap bone indices to nodes)
	PParams Desc;
	const nString& AnimDesc = GetEntity()->Get<nString>(Attr::AnimDesc);
	if (AnimDesc.IsValid()) Desc = DataSrv->LoadPRM(nString("game:Anim/") + AnimDesc + ".prm");

	if (Desc.isvalid())
	{
		for (int i = 0; i < Desc->GetCount(); ++i)
		{
			CParam& Prm = Desc->Get(i);
			CStrID ClipRsrcID = Prm.GetValue<CStrID>();
			Anim::PAnimClip Clip = SceneSrv->AnimationMgr.GetTypedResource(ClipRsrcID);
			if (!Clip.isvalid())
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
					LoadMocapClipFromNAX2(FileName, Bones, (Anim::CMocapClip*)Clip.get_unsafe());
				else if (FileName.CheckExtension("kfa"))
					LoadKeyframeClipFromKFA(FileName, (Anim::CKeyframeClip*)Clip.get_unsafe());
			}
			n_assert(Clip->IsLoaded());
			Clips.Add(Prm.GetName(), Clip);
		}
	}
//!!!to Activate() -

//!!!DBG TMP!
	if (Clips.Size())
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
			Bones.Add(pBone->GetIndex(), CStrID(Name.Get()));
			if (!pBone->IsTerminal()) AddChildrenToMapping(pNode, pRoot, Bones);
		}
	}
}
//---------------------------------------------------------------------

//!!!fading for non-looping anims
//!!!stopping
bool CPropAnimation::OnBeginFrame(const Events::CEventBase& Event)
{
	for (int i = 0; i < Tasks.Size(); ++i)
	{
		CAnimTask& Task = Tasks[i];

		//???to CAnimTask? there are "Task." _anywhere_!
		//we need to supply time only to the call

		if (Task.State == Task_Paused) continue;

		if (Task.State == Task_Starting)
		{
			for (int j = 0; j < Task.Ctlrs.Size(); ++j)
				Task.Ctlrs[j]->Activate(true);
			Task.State = Task_Running;
		}
		else if (Task.State == Task_Running)
			Task.CurrTime += (float)GameSrv->GetFrameTime() * Task.Speed;

		// Stop non-looping clips automatically
		if (!Task.Loop && Task.State == Task_Running)
		{
			//if ((Speed > 0.f && Task.CurrTime > Task.StopTimeBase) || (Speed < 0.f && Task.CurrTime < Task.StopTimeBase))
			if (Task.Speed * (Task.CurrTime - Task.StopTimeBase) > 0.f)
				Task.State = Task_Stopping;
		}

		// Apply optional fadein / fadeout, check for the end
		float RealWeight = Task.Weight;
		if (Task.State == Task_Stopping)
		{
			float CurrFadeOutTime = Task.CurrTime - Task.StopTimeBase;
			// if ((Speed > 0.f && CurrFadeOutTime >= Task.FadeOutTime) || (Speed < 0.f && CurrFadeOutTime <= Task.FadeOutTime))
			if (Task.Speed * (CurrFadeOutTime - Task.FadeOutTime) >= 0.f)
			{
				//!!!Task.Clear();
				Task.ClipID = CStrID::Empty;
				Task.Clip = NULL;
				Task.Ctlrs.Clear();
				continue;
			}

			RealWeight *= CurrFadeOutTime / Task.FadeOutTime;
		}
		else
		{
			//if ((Speed > 0.f && Task.CurrTime < Task.FadeInTime) || (Speed < 0.f && Task.CurrTime > Task.FadeInTime))
			if (Task.Speed * (Task.CurrTime - Task.FadeInTime) < 0.f)
				RealWeight *= (Task.CurrTime - Task.Offset) / (Task.FadeInTime - Task.Offset);
		}

		// Feed node controllers
		if (Task.Clip->IsA<Anim::CMocapClip>())
		{
			int KeyIndex;
			float IpolFactor;
			((Anim::CMocapClip*)Task.Clip.get())->GetSamplingParams(Task.CurrTime, Task.Loop, KeyIndex, IpolFactor);
			for (int j = 0; j < Task.Ctlrs.Size(); ++j)
				((Anim::CAnimControllerMocap*)Task.Ctlrs[j])->SetSamplingParams(KeyIndex, IpolFactor);
		}
		else if (Task.Clip->IsA<Anim::CKeyframeClip>())
		{
			float Time = Task.Clip->AdjustTime(Task.CurrTime, Task.Loop);
			for (int j = 0; j < Task.Ctlrs.Size(); ++j)
				((Anim::CAnimControllerKeyframe*)Task.Ctlrs[j])->SetTime(Time);
		}
	}

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

	CPropSceneNode* pProp = GetEntity()->FindProperty<CPropSceneNode>();
	if (!pProp || !pProp->GetNode()) return INVALID_INDEX; // Nothing to animate
	Scene::CSceneNode* pRoot = pProp->GetNode();

	int TaskID = INVALID_INDEX;
	CAnimTask* pTask = NULL;
	for (int i = 0; i < Tasks.Size(); ++i)
		if (!Tasks[i].ClipID.IsValid())
		{
			pTask = &Tasks[i];
			TaskID = i;
			break;
		}

	if (!pTask)
	{
		TaskID = Tasks.Size();
		pTask = Tasks.Reserve(1);
	}

	n_assert_dbg(!pTask->Ctlrs.Size());

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
		pTask->Ctlrs.Append(pNode->Controller.get_unsafe());

		//!!!
		// If still no blend controller, create and setup
		// Add child controller
		// Only blend controller allows to tune weight
		//????or weight to controller?
	}

	if (!pTask->Ctlrs.Size()) return INVALID_INDEX;

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
	pTask->State = Task_Starting;
	pTask->Loop = Loop;

	return TaskID;
}
//---------------------------------------------------------------------

void CPropAnimation::PauseAnim(DWORD TaskID, bool Pause)
{
	CAnimTask& Task = Tasks[TaskID];
	if (Task.State == Task_Stopping) return; //???what to do with Starting?
	if (Pause == (Task.State == Task_Paused)) return;
	for (int i = 0; i < Task.Ctlrs.Size(); ++i)
		Task.Ctlrs[i]->Activate(!Pause);
	Task.State = Pause ? Task_Paused : Task_Running;
}
//---------------------------------------------------------------------

void CPropAnimation::StopAnim(DWORD TaskID, float FadeOutTime)
{
	CAnimTask& Task = Tasks[TaskID];
	if (Task.State == Task_Stopping) return; //???what to do with Starting?

	if (FadeOutTime < 0.f) FadeOutTime = Task.FadeOutTime;

	if (!Task.Loop && FadeOutTime > Task.Clip->GetDuration() - Task.CurrTime)
		FadeOutTime = Task.Clip->GetDuration() - Task.CurrTime;

	if (FadeOutTime <= 0.f)
	{
		//!!!Task.Clear();
		Task.ClipID = CStrID::Empty;
		Task.Clip = NULL;
		Task.Ctlrs.Clear();
	}
	else
	{
		Task.State = Task_Stopping;
		Task.StopTimeBase = Task.CurrTime;
	}
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
