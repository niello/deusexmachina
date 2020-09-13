#include "TimelineTrackLoaderANM.h"
#include <Animation/PoseTrack.h>
#include <Animation/AnimatedPoseClip.h>
#include <Animation/AnimationClip.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>

namespace Resources
{

const Core::CRTTI& CTimelineTrackLoaderANM::GetResultType() const
{
	return DEM::Anim::CTimelineTrack::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CTimelineTrackLoaderANM::CreateResource(CStrID UID)
{
	//!!!need some postfix for the UID? Like #TL, to make distinction between TL and animation clip itself!

	auto AnimRsrc = _ResMgr.RegisterResource<DEM::Anim::CAnimationClip>(UID);
	auto Anim = AnimRsrc->ValidateObject<DEM::Anim::CAnimationClip>();
	if (!Anim) return nullptr;

	DEM::Anim::PAnimatedPoseClip Clip(n_new(DEM::Anim::CAnimatedPoseClip()));
	Clip->SetAnimationClip(Anim);
	DEM::Anim::PPoseTrack Track(n_new(DEM::Anim::CPoseTrack()));
	Track->AddClip(std::move(Clip), 0.f, Anim->GetDuration());

	return Track;
}
//---------------------------------------------------------------------

}
