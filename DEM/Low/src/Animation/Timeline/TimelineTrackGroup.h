#pragma once
#include <Animation/Timeline/TimelineTrack.h>

// Composite track that contains a group of child tracks (possibly composite too).
// Track group is useful to represent a timeline asset itself and to nest timeline assets.

namespace DEM::Anim
{

class CTimelineTrackGroup : public CTimelineTrack
{
public:

	virtual PTimelineTrack Clone() const override;
	virtual float          GetDuration() const override;
	virtual void           PlayInterval(float PrevTime, float CurrTime, bool IsLast) override;
	//virtual void           Visit(std::function<void(CTimelineTrack&)>&& Visitor) { Visitor(*this); + children }
	//virtual void           Visit(std::function<void(const CTimelineTrack&)>&& Visitor) const { Visitor(*this); + children }

	//???if timeline track base class manages clips, how to use this functionality here?

	// start time - offset in parent (if parent exists)
	// here because one offset is needed for the whole group.

	// vector<PTimelineTrack>

	// play interval - adjust by start time is made by clip playback logic, play interval on all child tracks
};

}
