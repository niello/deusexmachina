#include "PropAnimation.h"

#include <Game/Entity.h>
#include <Scene/SceneServer.h>
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

	//!!!MAP bone IDs to scene nodes!

	PROP_SUBSCRIBE_PEVENT(OnBeginFrame, CPropAnimation, OnBeginFrame);
}
//---------------------------------------------------------------------

void CPropAnimation::Deactivate()
{
	UNSUBSCRIBE_EVENT(OnBeginFrame);

	// Cleanup mappings, release resources

	Clips.Clear();

	Game::CProperty::Deactivate();
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

DWORD CPropAnimation::StartAnim(CStrID Clip, bool Loop, float Offset, float Speed, DWORD Priority,
								float Weight, float FadeInTime, float FadeOutTime)
{
	// For each output of this clip
	// Get corresponding node
	// If no node, skip
	// If node has no controller, attach new controller to it
	// If node has controller
	// Check priority and weight of curr & new controllers
	// Reject controllers above W = 1.0f in order of decreasing priority (the same as in blend ctlr, how to merge?)
	// If one controller is accepted, attach it to node
	// If more than one, create blend controller, setup it and attach
	// If blend controller exists, add new controller to it and it will decide what to do

	//!!!weights of least-priority tasks must not be clamped, since higher-priority task may be paused
	// and then resumed

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
