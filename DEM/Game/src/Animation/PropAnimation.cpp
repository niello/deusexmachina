#include "PropAnimation.h"

#include <Game/Entity.h>
#include <Game/GameServer.h> // For the time
#include <Scripting/PropScriptable.h>
#include <Scene/PropSceneNode.h>
#include <Scene/NodeControllerPriorityBlend.h>
#include <Scene/NodeControllerStatic.h>
#include <Animation/KeyframeClip.h>
#include <Animation/MocapClip.h>
#include <Animation/MocapClipLoaderNAX2.h> //!!!DBG TMP! NAX2 requires skin info!
#include <Frame/SkinAttribute.h> //!!!DBG TMP! NAX2 requires skin info!
#include <Animation/NodeControllerKeyframe.h>
#include <Animation/NodeControllerMocap.h>
#include <Render/SkinInfo.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <Data/ParamsUtils.h>
#include <IO/PathUtils.h>
#include <Core/Factory.h>

namespace Prop
{
__ImplementClass(Prop::CPropAnimation, 'PANM', Game::CProperty);
__ImplementPropertyStorage(CPropAnimation);

bool CPropAnimation::InternalActivate()
{
	CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
	if (pProp && pProp->IsActive()) InitSceneNodeModifiers(*pProp);

	CPropScriptable* pPropScr = GetEntity()->GetProperty<CPropScriptable>();
	if (pPropScr && pPropScr->IsActive()) EnableSI(*pPropScr);

	PROP_SUBSCRIBE_PEVENT(OnPropActivated, CPropAnimation, OnPropActivated);
	PROP_SUBSCRIBE_PEVENT(OnPropDeactivating, CPropAnimation, OnPropDeactivating);
	PROP_SUBSCRIBE_PEVENT(BeforeTransforms, CPropAnimation, BeforeTransforms);
	OK;
}
//---------------------------------------------------------------------

void CPropAnimation::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(OnPropActivated);
	UNSUBSCRIBE_EVENT(OnPropDeactivating);
	UNSUBSCRIBE_EVENT(BeforeTransforms);

	CPropScriptable* pPropScr = GetEntity()->GetProperty<CPropScriptable>();
	if (pPropScr && pPropScr->IsActive()) DisableSI(*pPropScr);

	CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
	if (pProp && pProp->IsActive()) TermSceneNodeModifiers(*pProp);

	Clips.Clear();
}
//---------------------------------------------------------------------

bool CPropAnimation::OnPropActivated(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
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

bool CPropAnimation::OnPropDeactivating(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
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

	// Remap bone indices to node relative pathes (NAX2-only)
	CDict<int, CStrID> Bones;
	Bones.Add(-1, CStrID::Empty);
	AddChildrenToMapping(Prop.GetNode(), Prop.GetNode(), Bones);

//!!!move it all to Activate() + (NAX2 loader requires ref-skeleton to remap bone indices to nodes)
	Data::PParams Desc;
	const CString& AnimDesc = GetEntity()->GetAttr<CString>(CStrID("AnimDesc"));
	if (AnimDesc.IsValid()) ParamsUtils::LoadParamsFromPRM(CString("GameAnim:") + AnimDesc + ".prm", Desc);

	if (Desc.IsValidPtr())
	{
		for (UPTR i = 0; i < Desc->GetCount(); ++i)
		{
			Data::CParam& Prm = Desc->Get(i);

			CStrID RsrcID = Prm.GetValue<CStrID>();
			CString RUID("Anims:");
			RUID += RsrcID.CStr();

			Resources::PResource Rsrc = ResourceMgr->RegisterResource<Anim::CAnimClip>(CStrID(RUID));

			//!!!DBG TMP! FIX!!!!!!!!!!!
			if (Rsrc->GetCreator()->GetResultType().IsDerivedFrom(Anim::CMocapClip::RTTI))
			{
				Frame::CSkinAttribute* pSkin = Prop.GetNode()->FindFirstAttribute<Frame::CSkinAttribute>();
				if (pSkin) ((Resources::CMocapClipLoaderNAX2*)Rsrc->GetCreator())->ReferenceSkinInfo = pSkin->GetSkinInfo();
			}

			Clips.Add(Prm.GetName(), Rsrc->ValidateObject<Anim::CAnimClip>());
		}
	}
//!!!to Activate() -

//!!!DBG TMP! some AI character controller must drive character animations
//for self-animated objects without AI can store info about current animations, it will help with save-load
//smart objects will restore their animation state from SO state
	if (Clips.GetCount())
		StartAnim(CStrID("Walk"), true, 0.f, 1.f, false, 10, 1.0f, 4.f, 4.f);
}
//---------------------------------------------------------------------

void CPropAnimation::TermSceneNodeModifiers(CPropSceneNode& Prop)
{
	for (UPTR i = 0; i < Tasks.GetCount(); ++i)
		Tasks[i].AnimTask.Stop(0.f);
	Tasks.Clear();
}
//---------------------------------------------------------------------

void CPropAnimation::AddChildrenToMapping(Scene::CSceneNode* pParent, Scene::CSceneNode* pRoot, CDict<int, CStrID>& Bones)
{
	for (UPTR i = 0; i < pParent->GetChildCount(); ++i)
	{
		Scene::CSceneNode* pNode = pParent->GetChild(i);
		/*Render::CBone* pBone = pNode->FindFirstAttribute<Render::CBone>();
		if (pBone)
		{
			static const CString StrDot(".");
			CString Name(pNode->GetName().CStr());
			Scene::CSceneNode* pCurrParent = pParent;
			while (pCurrParent && pCurrParent != pRoot)
			{
				Name = pCurrParent->GetName().CStr() + StrDot + Name;
				pCurrParent = pCurrParent->GetParent();
			}
			Bones.Add(pBone->GetIndex(), CStrID(Name.CStr()));
			if (!pBone->IsTerminal()) AddChildrenToMapping(pNode, pRoot, Bones);
		}*/
	}
}
//---------------------------------------------------------------------

bool CPropAnimation::BeforeTransforms(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	for (UPTR i = 0; i < Tasks.GetCount(); ++i)
	{
		CTask& Task = Tasks[i];
		Anim::CAnimTask& AnimTask = Task.AnimTask;

		if (AnimTask.IsEmpty()) continue;

		// Remove task if all its controllers were removed from target nodes
		UPTR j = 0;
		for (; j < AnimTask.Ctlrs.GetCount(); ++j)
			if (AnimTask.Ctlrs[j]->IsAttachedToNode()) break;

		if (j == AnimTask.Ctlrs.GetCount()) AnimTask.Stop(0.f);
		else
		{
			//!!!paused task must not resample tfms, but must be taken into account!
			//But Ctlr.Activate() controls both at a time.
			if (!Task.ManualControl && !Task.Paused)
				AnimTask.MoveCursorPos((float)GameSrv->GetFrameTime() * AnimTask.GetSpeed());
			AnimTask.Update();
		}

		if (AnimTask.IsEmpty())
		{
			Data::PParams P = n_new(Data::CParams(2));
			P->Set(CStrID("Clip"), Task.ClipID);
			P->Set<int>(CStrID("Task"), i);
			GetEntity()->FireEvent(CStrID("OnAnimStop"), P);
		}
	}

	OK;
}
//---------------------------------------------------------------------

IPTR CPropAnimation::StartAnim(CStrID ClipID, bool Loop, float CursorOffset, float Speed, bool ManualControl,
							  UPTR Priority, float Weight, float FadeInTime, float FadeOutTime)
{
	if (Speed == 0.f || Weight <= 0.f || Weight > 1.f || FadeInTime < 0.f || FadeOutTime < 0.f) return INVALID_INDEX;
	IPTR ClipIdx = Clips.FindIndex(ClipID);
	if (ClipIdx == INVALID_INDEX) return INVALID_INDEX; // Invalid task ID
	Anim::PAnimClip Clip = Clips.ValueAt(ClipIdx);
	if (!Clip->GetSamplerCount() || !Clip->GetDuration()) return INVALID_INDEX;
	if (!Loop && (CursorOffset < 0.f || CursorOffset > Clip->GetDuration())) return INVALID_INDEX;

	CPropSceneNode* pPropNode = GetEntity()->GetProperty<CPropSceneNode>();
	if (!pPropNode || !pPropNode->GetNode()) return INVALID_INDEX; // Nothing to animate

	IPTR TaskID = INVALID_INDEX;
	CTask* pTask = nullptr;
	for (UPTR i = 0; i < Tasks.GetCount(); ++i)
		if (Tasks[i].AnimTask.IsEmpty())
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

	bool NeedWeight = (Weight < 1.f || FadeInTime > 0.f || FadeOutTime > 0.f);
	bool BlendingIsNotNecessary = (!NeedWeight && Priority == AnimPriority_Default);

	UPTR FreePoseLockerIdx = 0;

	n_assert_dbg(!pTask->AnimTask.Ctlrs.GetCount());
	Scene::PNodeController* ppCtlr = pTask->AnimTask.Ctlrs.Reserve(Clip->GetSamplerCount());
	for (UPTR i = 0; i < Clip->GetSamplerCount(); ++i, ++ppCtlr)
	{
		// Get controller target node
		Scene::CSceneNode* pNode = pPropNode->GetChildNode(Clip->GetSamplerTarget(i));
		if (!pNode) continue;

		*ppCtlr = Clip->CreateController(i); //???!!!PERF: use pools inside?!

		if (BlendingIsNotNecessary) // && (!BlendCtlr.IsValid() || !BlendCtlr->GetSourceCount()))
		{
			// Discards locked pose. Decide when this is desirable and when it is not.
			pNode->SetController(*ppCtlr);
		}
		else
		{
			Scene::PNodeControllerPriorityBlend BlendCtlr;
			if (pNode->GetController() && pNode->GetController()->IsA<Scene::CNodeControllerPriorityBlend>())
				BlendCtlr = (Scene::CNodeControllerPriorityBlend*)pNode->GetController();
			
			if (BlendCtlr.IsNullPtr())
			{
				BlendCtlr = n_new(Scene::CNodeControllerPriorityBlend);
				if (pNode->GetController()) BlendCtlr->AddSource(*pNode->GetController(), AnimPriority_Default, 1.f);
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
				BlendCtlr->AddSource(*PoseLock, AnimPriority_TheLeast, 1.f);
				PoseLock->Activate(true); //???where to deactivate? when all tasks were stopped and it is the only source
			}

			BlendCtlr->AddSource(**ppCtlr, Priority, Weight);

			pNode->SetController(BlendCtlr);
			BlendCtlr->Activate(true);
		}
	}

	if (!pTask->AnimTask.Ctlrs.GetCount()) return INVALID_INDEX;

	pTask->ClipID = ClipID;
	pTask->ManualControl = ManualControl;
	pTask->Paused = false;
	pTask->AnimTask.pEventDisp = GetEntity();
	pTask->AnimTask.Params = n_new(Data::CParams(2));
	pTask->AnimTask.Params->Set(CStrID("Clip"), ClipID);
	pTask->AnimTask.Params->Set(CStrID("Task"), (int)TaskID);
	pTask->AnimTask.Init(Clip, Loop, CursorOffset, Speed, Weight, FadeInTime, FadeOutTime);

	GetEntity()->FireEvent(CStrID("OnAnimStart"), pTask->AnimTask.Params);

	return TaskID;
}
//---------------------------------------------------------------------

void CPropAnimation::StopAnim(UPTR TaskID, float FadeOutTime)
{
	CTask& Task = Tasks[TaskID];
	Task.AnimTask.Stop(FadeOutTime);
	if (Task.AnimTask.IsEmpty()) //!!!now can never happen due to Task_LastFrame!
	{
		Data::PParams P = n_new(Data::CParams(2));
		P->Set(CStrID("Clip"), Task.ClipID);
		P->Set(CStrID("Task"), (int)TaskID);
		GetEntity()->FireEvent(CStrID("OnAnimStop"), P);
	}
}
//---------------------------------------------------------------------

// Doesn't support weights and blending for now, use animation tasks for that
bool CPropAnimation::SetPose(CStrID ClipID, float CursorPos, bool WrapPos) const
{
	IPTR ClipIdx = Clips.FindIndex(ClipID);
	if (ClipIdx == INVALID_INDEX) FAIL;
	Anim::PAnimClip Clip = Clips.ValueAt(ClipIdx);
	if (!Clip->GetSamplerCount() || !Clip->GetDuration()) FAIL;

	CPropSceneNode* pPropNode = GetEntity()->GetProperty<CPropSceneNode>();
	if (!pPropNode || !pPropNode->GetNode()) FAIL; // Nothing to animate
 
	int KeyIndex;
	float IpolFactor, AdjTime;
	bool IsMocap = Clip->IsA<Anim::CMocapClip>();
	bool IsKeyframe = !IsMocap && Clip->IsA<Anim::CKeyframeClip>();
	if (IsMocap)
		((Anim::CMocapClip*)Clip.Get())->GetSamplingParams(CursorPos, WrapPos, KeyIndex, IpolFactor);
	else if (IsKeyframe)
		AdjTime = Clip->AdjustCursorPos(CursorPos, WrapPos);

	for (UPTR i = 0; i < Clip->GetSamplerCount(); ++i)
	{
		Scene::CSceneNode* pNode = pPropNode->GetChildNode(Clip->GetSamplerTarget(i));
		if (!pNode) continue;

		Scene::PNodeController pCtlr = Clip->CreateController(i); //???!!!PERF: use pools inside?!

		if (IsMocap)
			((Anim::CNodeControllerMocap*)pCtlr.Get())->SetSamplingParams(KeyIndex, IpolFactor);
		else if (IsKeyframe)
			((Anim::CNodeControllerKeyframe*)pCtlr.Get())->SetTime(AdjTime);

		// Controller may animate not all channels, so we want other channels to keep their value
		Math::CTransform Tfm = pNode->GetLocalTransform();
		pCtlr->ApplyTo(Tfm);
		pNode->SetLocalTransform(Tfm);
	}

	OK;
}
//---------------------------------------------------------------------

float CPropAnimation::GetAnimLength(CStrID ClipID) const
{
	IPTR ClipIdx = Clips.FindIndex(ClipID);
	if (ClipIdx == INVALID_INDEX) return 0.f;
	return Clips.ValueAt(ClipIdx)->GetDuration();
}
//---------------------------------------------------------------------

}