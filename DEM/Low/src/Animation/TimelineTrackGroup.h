#pragma once
#include <Animation/TimelineTrack.h>

// Composite track that contains a group of child tracks (possibly composite too).
// Track group is useful to represent a timeline asset itself and to nest timeline assets.

namespace DEM::Anim
{

class CTimelineTrackGroup : public CTimelineTrack
{
public:

	//???if timeline track base class manages clips, how to use this functionality here?

	// start time - offset in parent (if parent exists)
	// here because one offset is needed for the whole group.

	// vector<PTimelineTrack>

	// play interval - adjust by start time, play interval on all child tracks
};

}
