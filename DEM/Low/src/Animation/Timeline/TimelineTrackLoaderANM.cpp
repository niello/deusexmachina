#include "TimelineTrackLoaderANM.h"
#include <Animation/Timeline/PoseTrack.h>
#include <Animation/Timeline/AnimatedPoseClip.h>
#include <Animation/AnimationClip.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>

namespace Resources
{

const DEM::Core::CRTTI& CTimelineTrackLoaderANM::GetResultType() const
{
	return DEM::Anim::CTimelineTrack::RTTI;
}
//---------------------------------------------------------------------

DEM::Core::PObject CTimelineTrackLoaderANM::CreateResource(CStrID UID)
{
	// Requires #TL sub-ID to distinguish from animation clip resource
	const char* pSubId = strchr(UID.CStr(), '#');
	if (!pSubId || strcmp(pSubId, "#TL")) return nullptr;

	std::string AnimUID(UID.CStr(), pSubId);
	auto AnimRsrc = _ResMgr.RegisterResource<DEM::Anim::CAnimationClip>(AnimUID.c_str());
	auto Anim = AnimRsrc->ValidateObject<DEM::Anim::CAnimationClip>();
	if (!Anim) return nullptr;

	DEM::Anim::PAnimatedPoseClip Clip(n_new(DEM::Anim::CAnimatedPoseClip()));
	Clip->SetAnimationClip(Anim);
	DEM::Anim::PPoseTrack Track(n_new(DEM::Anim::CPoseTrack(CStrID("ObjectSkeletonRoot"))));
	Track->AddClip(std::move(Clip), 0.f, Anim->GetDuration());
	Track->RefreshSkeletonInfo();

	//!!!could also load CEventClip right from anm!
	//also can make CEventClipLoaderANM just like this one for loading events from ANM as a separate event clip with #Events or like that

	return Track;
}
//---------------------------------------------------------------------

}
