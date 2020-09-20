#include "TimelineTrackGroup.h"

namespace DEM::Anim
{

PTimelineTrack CTimelineTrackGroup::Clone() const
{
	NOT_IMPLEMENTED;
	return nullptr;
}
//---------------------------------------------------------------------

float CTimelineTrackGroup::GetDuration() const
{
	float MaxDuration = 0.f;
	// get max duration from nested tracks
	return MaxDuration;
}
//---------------------------------------------------------------------

void CTimelineTrackGroup::PlayInterval(float PrevTime, float CurrTime, bool IsLast)
{
	// time must be already adjusted if inside clip?
	// play interval on each nested track in order
}
//---------------------------------------------------------------------

}
