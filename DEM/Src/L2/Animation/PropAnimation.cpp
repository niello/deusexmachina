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
	bool LoadMocapClipFromNAX2(const nString& FileName, PMocapClip OutClip);
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
					LoadMocapClipFromNAX2(FileName, (Anim::CMocapClip*)Clip.get_unsafe());
				else if (FileName.CheckExtension("kfa"))
					LoadKeyframeClipFromKFA(FileName, (Anim::CKeyframeClip*)Clip.get_unsafe());
			}
			n_assert(Clip->IsLoaded());
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
			if (Task.Clip->IsA<Anim::CMocapClip>())
			{
				int KeyIndex;
				float IpolFactor;
				((Anim::CMocapClip*)Task.Clip.get())->GetSamplingParams(Task.CurrTime, Task.Loop, KeyIndex, IpolFactor);

				for (int j = 0; j < Task.Ctlrs.Size(); ++j)
					((Anim::CAnimControllerMocap*)Task.Ctlrs[j])->SetSamplingParams(KeyIndex, IpolFactor);
			}
			else
			{
				float Time = Task.Clip->AdjustTime(Task.CurrTime, Task.Loop);
				for (int j = 0; j < Task.Ctlrs.Size(); ++j)
					((Anim::CAnimControllerKeyframe*)Task.Ctlrs[j])->SetTime(Time);
			}
		}
	}

	OK;
}
//---------------------------------------------------------------------

int CPropAnimation::StartAnim(CStrID ClipID, bool Loop, float Offset, float Speed, DWORD Priority,
							  float Weight, float FadeInTime, float FadeOutTime)
{
	int ClipIdx = Clips.FindIndex(ClipID);
	if (ClipIdx == INVALID_INDEX) return INVALID_INDEX; // Invalid task ID
	Anim::PAnimClip Clip = Clips.ValueAtIndex(ClipIdx);
	if (!Clip->GetSamplerCount()) return INVALID_INDEX;

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

	for (DWORD i = 0; i < Clip->GetSamplerCount(); ++i)
	{
		Anim::CBoneID Target = Clip->GetSamplerTarget(i);
		int NodeIdx = Nodes.FindIndex(Target);
		if (NodeIdx == INVALID_INDEX) continue;
		Scene::CSceneNode* pNode = Nodes.ValueAtIndex(NodeIdx);
		pNode->Controller = Clip->CreateController(i);
		pTask->Ctlrs.Append(pNode->Controller.get_unsafe());

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
