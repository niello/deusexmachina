#include "PropAnimation.h"

#include <Game/Entity.h>
#include <Game/GameServer.h> // For the time
#include <Scene/PropSceneNode.h>
#include <Scene/SceneServer.h>
#include <Scene/Bone.h>
#include <Animation/AnimControllerMocap.h> //???virtualize controller creation in clip later?
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
	bool LoadMocapClipFromNAX2(const nString& FileName, PMocapClip OutClip);
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

	PParams Desc;
	const nString& AnimDesc = GetEntity()->Get<nString>(Attr::AnimDesc);
	if (AnimDesc.IsValid()) Desc = DataSrv->LoadPRM(nString("game:Anim/") + AnimDesc + ".prm");

	if (Desc.isvalid())
	{
		for (int i = 0; i < Desc->GetCount(); ++i)
		{
			CParam& Prm = Desc->Get(i);
			CStrID ClipRsrcID = Prm.GetValue<CStrID>();
			Anim::PMocapClip Clip = SceneSrv->AnimationMgr.GetTypedResource(ClipRsrcID);
			if (!Clip->IsLoaded()) n_assert(LoadMocapClipFromNAX2(Clip->GetUID().CStr(), Clip));
			Clips.Add(Prm.GetName(), Clip);
		}
	}

	PROP_SUBSCRIBE_PEVENT(OnPropsActivated, CPropAnimation, OnPropsActivated);
	PROP_SUBSCRIBE_PEVENT(OnBeginFrame, CPropAnimation, OnBeginFrame);
}
//---------------------------------------------------------------------

void CPropAnimation::Deactivate()
{
	UNSUBSCRIBE_EVENT(OnPropsActivated);
	UNSUBSCRIBE_EVENT(OnBeginFrame);

	//!!!abort all tasks and destroy controllers!
	//Tasks.Clear();

	Clips.Clear();
	Nodes.Clear();

	Game::CProperty::Deactivate();
}
//---------------------------------------------------------------------

bool CPropAnimation::OnPropsActivated(const Events::CEventBase& Event)
{
	CPropSceneNode* pProp = GetEntity()->FindProperty<CPropSceneNode>();
	if (!pProp || !pProp->GetNode()) OK; // Nothing to animate

	//Nodes.Add(CStrID::Empty, pProp->GetNode());
	Nodes.Add(-1, pProp->GetNode());
	AddChildrenToMapping(pProp->GetNode());

//!!!DBG TMP!
	StartAnim(CStrID("Walk"), true, 0.f, 1.f, 10, 1.f, 0.f, 0.f);

	OK;
}
//---------------------------------------------------------------------

void CPropAnimation::AddChildrenToMapping(Scene::CSceneNode* pParentNode)
{
	for (DWORD i = 0; i < pParentNode->GetChildCount(); ++i)
	{
		Scene::CSceneNode* pNode = pParentNode->GetChild(i);
		Scene::CBone* pBone = pNode->FindFirstAttr<Scene::CBone>();
		if (pBone)
		{
			//Nodes.Add(pNode->GetName(), pNode);
			Nodes.Add(pBone->GetIndex(), pNode);
			if (!pBone->IsTerminal()) AddChildrenToMapping(pNode);
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

		if (Task.State == Task_Starting)
		{
			for (int j = 0; j < Task.Ctlrs.Size(); ++j)
				Task.Ctlrs[j]->Activate(true);
			Task.State = Task_Active;
		}
		else if (Task.State == Task_Active)
		{
			Task.CurrTime += (float)GameSrv->GetFrameTime() * Task.Speed;
		}

		//if time is in fade in sector, calc fade in weight

		//if not looping, check fadeout sector or stop

		if (Task.State == Task_Active)
		{
			int KeyIndex;
			float IpolFactor;
			Task.Clip->GetSamplingParams(Task.CurrTime, Task.Loop, KeyIndex, IpolFactor);

			//!!!if not mocap clip, set time directly to controllers!
			for (int j = 0; j < Task.Ctlrs.Size(); ++j)
				((Anim::CAnimControllerMocap*)Task.Ctlrs[j])->SetSamplingParams(KeyIndex, IpolFactor);
		}
	}

	OK;
}
//---------------------------------------------------------------------

int CPropAnimation::StartAnim(CStrID ClipID, bool Loop, float Offset, float Speed, DWORD Priority,
							  float Weight, float FadeInTime, float FadeOutTime)
{
	//!!!SEPARATE TO MOCAP AND KF CODE!
	//???virtual Clip->CreateController(NodeID/Sampler)?

	int ClipIdx = Clips.FindIndex(ClipID);
	if (ClipIdx == INVALID_INDEX) return INVALID_INDEX; // Invalid task ID
	Anim::PMocapClip Clip = Clips.ValueAtIndex(ClipIdx);

	const Anim::CMocapClip::CSamplerList& Samplers = Clip->GetSamplerList();
	if (!Samplers.Size()) return INVALID_INDEX; // Invalid task ID

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

	pTask->ClipID = ClipID;
	pTask->Clip = Clip;
	pTask->CurrTime = Offset;
	pTask->FadeInTime = FadeInTime;
	pTask->FadeOutTime = FadeOutTime;
	pTask->State = Task_Starting;
	pTask->Loop = Loop;
	pTask->Speed = Speed;

	//???If task is not looping, clamp fadeout time to fit into the clip length (with speed factor)
	//see StopAnim

	for (int i = 0; i < Samplers.Size(); ++i)
	{
		int NodeIdx = Nodes.FindIndex(Samplers.KeyAtIndex(i));
		if (NodeIdx == INVALID_INDEX) continue;
		Scene::CSceneNode* pNode = Nodes.ValueAtIndex(NodeIdx);

		//???virtual Clip->CreateController(NodeID/Sampler)?
		Anim::PAnimControllerMocap Ctlr = n_new(Anim::CAnimControllerMocap);
		Ctlr->SetSampler(&Samplers.ValueAtIndex(i));
		pNode->Controller = Ctlr;
		pTask->Ctlrs.Append(Ctlr.get_unsafe());

		//!!!
		// If still no blend controller, create and setup
		// Add child controller
		// Only blend controller allows to tune weight
		//????or weight to controller?
	}

	return TaskID;
}
//---------------------------------------------------------------------

void CPropAnimation::PauseAnim(DWORD TaskID, bool Pause)
{
	CAnimTask& Task = Tasks[TaskID];
	if (Pause == (Task.State == Task_Paused)) return;
	for (int i = 0; i < Task.Ctlrs.Size(); ++i)
		Task.Ctlrs[i]->Activate(!Pause);
	Task.State = Pause ? Task_Paused : Task_Active;
}
//---------------------------------------------------------------------

void CPropAnimation::StopAnim(DWORD TaskID, float FadeOutTime)
{
	// If task is not looping, clamp fadeout time to fit into the clip length (with speed factor)
	// If no more time, set fadeout time to 0
	// If fadeout time > 0, schedule stop for this task and disallow pause on it
	// else stop right here:
	// Deactivate and destroy all controllers of the task
	// If it leaves some blend controller with one controller inside, collapse it
}
//---------------------------------------------------------------------

} // namespace Properties
