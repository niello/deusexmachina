#include "PropAnimation.h"

#include <Game/Entity.h>
#include <Game/GameServer.h> // For the time
#include <Scripting/PropScriptable.h>
#include <Scene/PropSceneNode.h>
#include <Scene/SceneServer.h>
#include <Scene/Bone.h>
#include <Scene/NodeControllerPriorityBlend.h>
#include <Scene/NodeControllerStatic.h>
#include <Animation/KeyframeClip.h>
#include <Animation/MocapClip.h>
#include <Data/DataServer.h>

namespace Anim
{
	bool LoadMocapClipFromNAX2(const CString& FileName, const CDict<int, CStrID>& BoneToNode, PMocapClip OutClip);
	bool LoadKeyframeClipFromKFA(const CString& FileName, PKeyframeClip OutClip);
}

namespace Prop
{
__ImplementClass(Prop::CPropAnimation, 'PANM', Game::CProperty);
__ImplementPropertyStorage(CPropAnimation);

using namespace Data;

bool CPropAnimation::InternalActivate()
{
	CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
	if (pProp && pProp->IsActive()) InitSceneNodeModifiers(*pProp);

	CPropScriptable* pPropScr = GetEntity()->GetProperty<CPropScriptable>();
	if (pPropScr && pPropScr->IsActive()) EnableSI(*pPropScr);

	PROP_SUBSCRIBE_PEVENT(OnPropActivated, CPropAnimation, OnPropActivated);
	PROP_SUBSCRIBE_PEVENT(OnPropDeactivating, CPropAnimation, OnPropDeactivating);
	PROP_SUBSCRIBE_PEVENT(OnBeginFrame, CPropAnimation, OnBeginFrame);
	OK;
}
//---------------------------------------------------------------------

void CPropAnimation::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(OnPropActivated);
	UNSUBSCRIBE_EVENT(OnPropDeactivating);
	UNSUBSCRIBE_EVENT(OnBeginFrame);

	CPropScriptable* pPropScr = GetEntity()->GetProperty<CPropScriptable>();
	if (pPropScr && pPropScr->IsActive()) DisableSI(*pPropScr);

	CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
	if (pProp && pProp->IsActive()) TermSceneNodeModifiers(*pProp);

	Clips.Clear();
}
//---------------------------------------------------------------------

bool CPropAnimation::OnPropActivated(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (pProp->IsA<CPropSceneNode>())
	{
		InitSceneNodeModifiers(*(CPropSceneNode*)pProp);
		OK;
	}

	if (pProp->IsA<CPropScriptable>())
	{
		EnableSI(*(CPropScriptable*)pProp);
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropAnimation::OnPropDeactivating(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (pProp->IsA<CPropSceneNode>())
	{
		TermSceneNodeModifiers(*(CPropSceneNode*)pProp);
		OK;
	}

	if (pProp->IsA<CPropScriptable>())
	{
		DisableSI(*(CPropScriptable*)pProp);
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

void CPropAnimation::InitSceneNodeModifiers(CPropSceneNode& Prop)
{
	if (!Prop.GetNode()) return; // Nothing to animate

	// Remap bone indices to node relative pathes
	CDict<int, CStrID> Bones;
	Bones.Add(-1, CStrID::Empty);
	AddChildrenToMapping(Prop.GetNode(), Prop.GetNode(), Bones);

//!!!to Activate() + (NAX2 loader requires ref-skeleton to remap bone indices to nodes)
	PParams Desc;
	const CString& AnimDesc = GetEntity()->GetAttr<CString>(CStrID("AnimDesc"));
	if (AnimDesc.IsValid()) Desc = DataSrv->LoadPRM(CString("GameAnim:") + AnimDesc + ".prm");

	if (Desc.IsValid())
	{
		for (int i = 0; i < Desc->GetCount(); ++i)
		{
			CParam& Prm = Desc->Get(i);

			CStrID ClipRsrcID = Prm.GetValue<CStrID>();
			CString FileName("Anims:");
			FileName += ClipRsrcID.CStr();
			bool IsMocap = FileName.CheckExtension("mca") || FileName.CheckExtension("nax2");

			Anim::PAnimClip Clip;
			if (IsMocap)
				Clip = SceneSrv->AnimationMgr.GetOrCreateTypedResource<Anim::CMocapClip>(ClipRsrcID);
			else
				Clip = SceneSrv->AnimationMgr.GetOrCreateTypedResource<Anim::CKeyframeClip>(ClipRsrcID);

			if (!Clip->IsLoaded())
			{
				if (IsMocap)
					LoadMocapClipFromNAX2(FileName, Bones, (Anim::CMocapClip*)Clip.GetUnsafe());
				else
					LoadKeyframeClipFromKFA(FileName, (Anim::CKeyframeClip*)Clip.GetUnsafe());
			}
			n_assert(Clip->IsLoaded());

			Clips.Add(Prm.GetName(), Clip);
		}
	}
//!!!to Activate() -

//!!!DBG TMP! some AI character controller must drive character animations
//for self-animated objects without AI can store info about current animations, it will help with save-load
	if (Clips.GetCount())
		StartAnim(CStrID("Walk"), true, 0.f, 1.f, 10, 1.0f, 4.f, 4.f);
}
//---------------------------------------------------------------------

void CPropAnimation::TermSceneNodeModifiers(CPropSceneNode& Prop)
{
	for (int i = 0; i < Tasks.GetCount(); ++i)
		Tasks[i].Stop(0.f);
	Tasks.Clear();
	Nodes.Clear();
}
//---------------------------------------------------------------------

void CPropAnimation::AddChildrenToMapping(Scene::CSceneNode* pParent, Scene::CSceneNode* pRoot, CDict<int, CStrID>& Bones)
{
	for (DWORD i = 0; i < pParent->GetChildCount(); ++i)
	{
		Scene::CSceneNode* pNode = pParent->GetChild(i);
		Scene::CBone* pBone = pNode->FindFirstAttr<Scene::CBone>();
		if (pBone)
		{
			static const CString StrDot(".");
			CString Name = pNode->GetName().CStr();
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
	{
		Anim::CAnimTask& Task = Tasks[i];

		if (Task.IsEmpty()) continue;

		int j = 0;
		for (; j < Task.Ctlrs.GetCount(); ++j)
			if (Task.Ctlrs[j]->IsAttachedToNode()) break;

		// Remove task if all its controllers were removed from target nodes
		if (j == Task.Ctlrs.GetCount())
		{
			Task.Stop(0.f);
			Data::PParams P = n_new(Data::CParams(1));
			P->Set(CStrID("Clip"), Task.ClipID);
			GetEntity()->FireEvent(CStrID("OnAnimStop"), P);
		}
		else
		{
			Task.Update(FrameTime);
			if (Task.IsEmpty())
			{
				Data::PParams P = n_new(Data::CParams(1));
				P->Set(CStrID("Clip"), Task.ClipID);
				GetEntity()->FireEvent(CStrID("OnAnimStop"));
			}
		}
	}
	OK;
}
//---------------------------------------------------------------------

int CPropAnimation::StartAnim(CStrID ClipID, bool Loop, float Offset, float Speed, DWORD Priority,
							  float Weight, float FadeICTime, float FadeOutTime)
{
	if (Speed == 0.f || Weight <= 0.f || Weight > 1.f || FadeICTime < 0.f || FadeOutTime < 0.f) return INVALID_INDEX;
	int ClipIdx = Clips.FindIndex(ClipID);
	if (ClipIdx == INVALID_INDEX) return INVALID_INDEX; // Invalid task ID
	Anim::PAnimClip Clip = Clips.ValueAt(ClipIdx);
	if (!Clip->GetSamplerCount() || !Clip->GetDuration()) return INVALID_INDEX;
	if (!Loop && (Offset < 0.f || Offset > Clip->GetDuration())) return INVALID_INDEX;

	CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
	if (!pProp || !pProp->GetNode()) return INVALID_INDEX; // Nothing to animate
	Scene::CSceneNode* pRoot = pProp->GetNode();

	int TaskID = INVALID_INDEX;
	Anim::CAnimTask* pTask = NULL;
	for (int i = 0; i < Tasks.GetCount(); ++i)
		if (Tasks[i].IsEmpty())
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

	bool NeedWeight = (Weight < 1.f || FadeICTime > 0.f || FadeOutTime > 0.f);
	bool BlendingIsNotNecessary = (!NeedWeight && Priority == 0);

	int FreePoseLockerIdx = 0;

	n_assert_dbg(!pTask->Ctlrs.GetCount());
	Scene::PNodeController* ppCtlr = pTask->Ctlrs.Reserve(Clip->GetSamplerCount());
	for (DWORD i = 0; i < Clip->GetSamplerCount(); ++i, ++ppCtlr)
	{
		// Get controller target node
		Scene::CSceneNode* pNode;
		CStrID Target = Clip->GetSamplerTarget(i);
		int NodeIdx = Nodes.FindIndex(Target);
		if (NodeIdx == INVALID_INDEX)
		{
			pNode = pRoot->GetChild(Target.CStr());
			if (pNode) Nodes.Add(Target, pNode);
			else continue;
		}
		else pNode = Nodes.ValueAt(NodeIdx);

		*ppCtlr = Clip->CreateController(i);

		Scene::PNodeControllerPriorityBlend BlendCtlr;
		if (pNode->GetController() && pNode->GetController()->IsA<Scene::CNodeControllerPriorityBlend>())
			BlendCtlr = (Scene::CNodeControllerPriorityBlend*)pNode->GetController();

		if (BlendingIsNotNecessary && (!BlendCtlr.IsValid() || !BlendCtlr->GetSourceCount()))
		{
			pNode->SetController(*ppCtlr);
		}
		else
		{
			if (!BlendCtlr.IsValid())
			{
				BlendCtlr = n_new(Scene::CNodeControllerPriorityBlend);
				if (pNode->GetController()) BlendCtlr->AddSource(*pNode->GetController(), 0, 1.f);
			}

			// If blend controller is new, we capture current pose to it with full weight but the least priority,
			// so blending will always be correct, using this pose as a base.
			if (!BlendCtlr->GetSourceCount() && NeedWeight)
			{
				while (FreePoseLockerIdx < BasePose.GetCount() && BasePose[FreePoseLockerIdx]->IsAttachedToNode())
					++FreePoseLockerIdx;
				Scene::PNodeControllerStatic& PoseLock =
					(FreePoseLockerIdx < BasePose.GetCount()) ?
					BasePose[FreePoseLockerIdx] :
					*BasePose.Add(n_new(Scene::CNodeControllerStatic));

				if (!pNode->IsLocalTransformValid()) pNode->UpdateLocalFromWorld();
				PoseLock->SetStaticTransform(pNode->GetLocalTransform(), true);
				BlendCtlr->AddSource(*PoseLock, 0, 1.f);
				PoseLock->Activate(true); //???where to deactivate? when all tasks were stopped and it is the only source
			}

			BlendCtlr->AddSource(**ppCtlr, Priority, Weight);

			pNode->SetController(BlendCtlr);
			BlendCtlr->Activate(true);
		}
	}

	if (!pTask->Ctlrs.GetCount()) return INVALID_INDEX;

	if (!Loop)
	{
		float RealDuration = Clip->GetDuration() / n_fabs(Speed);
		if (FadeICTime + FadeOutTime > RealDuration)
		{
			FadeOutTime = n_max(0.f, RealDuration - FadeICTime);
			FadeICTime = RealDuration - FadeOutTime;
		}
	}

	FadeICTime *= Speed;
	FadeOutTime *= Speed;

	if (!Loop)
	{
		if (Speed > 0.f) pTask->StopTimeBase = Clip->GetDuration() - FadeOutTime;
		else
		{
			pTask->StopTimeBase = -FadeOutTime;
			if (Offset == 0.f)
			{
				n_printf("Anim,Warning: Reverse non-looping animation with offset = 0 will not be played, because offset 0 is its end");
				Offset = Clip->GetDuration();
			}
		}
	}

	pTask->ClipID = ClipID;
	pTask->Clip = Clip;
	pTask->Offset = Offset;
	pTask->CurrTime = Offset;
	pTask->Speed = Speed;
	pTask->Priority = Priority;
	pTask->Weight = Weight;
	pTask->FadeICTime = Offset + FadeICTime;	// Get a point in time because we know the start time
	pTask->FadeOutTime = FadeOutTime;			// Remember only the length, because we don't know the end time
	pTask->State = Anim::CAnimTask::Task_Starting;
	pTask->Loop = Loop;
	pTask->pEventDisp = GetEntity();
	pTask->Params = n_new(Data::CParams);
	pTask->Params->Set(CStrID("Clip"), ClipID);

	Data::PParams P = n_new(Data::CParams(1));
	P->Set(CStrID("Clip"), ClipID);
	GetEntity()->FireEvent(CStrID("OnAnimStart"));

	return TaskID;
}
//---------------------------------------------------------------------

void CPropAnimation::StopAnim(DWORD TaskID, float FadeOutTime)
{
	Anim::CAnimTask& Task = Tasks[TaskID];
	Task.Stop(FadeOutTime);
	if (Task.IsEmpty())
	{
		Data::PParams P = n_new(Data::CParams(1));
		P->Set(CStrID("Clip"), Task.ClipID);
		GetEntity()->FireEvent(CStrID("OnAnimStop"), P);
	}
}
//---------------------------------------------------------------------

float CPropAnimation::GetAnimLength(CStrID ClipID) const
{
	int ClipIdx = Clips.FindIndex(ClipID);
	if (ClipIdx == INVALID_INDEX) return 0.f;
	return Clips.ValueAt(ClipIdx)->GetDuration();
}
//---------------------------------------------------------------------

}