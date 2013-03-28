#include "PropAnimation.h"

#include <Game/Entity.h>
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
	//StartAnim(CStrID("Walk"), true, 0.f, 1.f, 10, 1.f, 0.f, 0.f);

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

bool CPropAnimation::OnBeginFrame(const Events::CEventBase& Event)
{
//foreach task
//if this task uses mocap clip
//	KeyFrame, Factor = TimeToIndex(Time, Loop(?))
//	Init all mocap controllers of this task with KeyFrame & Factor
//else
//	Init all clip controllers of this task with Time & Loop(?)
	OK;
}
//---------------------------------------------------------------------

DWORD CPropAnimation::StartAnim(CStrID ClipID, bool Loop, float Offset, float Speed, DWORD Priority,
								float Weight, float FadeInTime, float FadeOutTime)
{
	//!!!SEPARATE TO MOCAP AND KF CODE!
	//???virtual Clip->CreateController(NodeID/Sampler)?

	int ClipIdx = Clips.FindIndex(ClipID);
	if (ClipIdx == INVALID_INDEX) return INVALID_INDEX; // Invalid task ID
	Anim::PMocapClip Clip = Clips.ValueAtIndex(ClipIdx);

	const Anim::CMocapClip::CSamplerList& Samplers = Clip->GetSamplerList();
	if (!Samplers.Size()) return INVALID_INDEX; // Invalid task ID

	for (int i = 0; i < Samplers.Size(); ++i)
	{
		int NodeIdx = Nodes.FindIndex(Samplers.KeyAtIndex(i));
		if (NodeIdx == INVALID_INDEX) continue;
		Scene::CSceneNode* pNode = Nodes.ValueAtIndex(NodeIdx);

		//???virtual Clip->CreateController(NodeID/Sampler)?
		Anim::PAnimControllerMocap Ctlr = n_new(Anim::CAnimControllerMocap);
		Ctlr->SetSampler(&Samplers.ValueAtIndex(i));

		// If still no blend controller, create and setup
		// Add child controller

		// Set controller to node (fast debug code)
		pNode->Controller = Ctlr;
	}

	// Obtain free task slot
	// Fill task record
	// Return correct task ID

	return INVALID_INDEX;
}
//---------------------------------------------------------------------

void CPropAnimation::PauseAnim(DWORD TaskID, bool Pause)
{
	// Activate/deactivate all controllers of the task
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
