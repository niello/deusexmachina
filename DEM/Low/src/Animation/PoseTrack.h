#pragma once
#include <Animation/TimelineTrack.h>

// Track that outputs local SRT transform to an abstract pose output.
// This is the most common track type, it is used for skeletal animation.

namespace DEM::Anim
{

class CPoseTrack : public ITimelineTrack
{
public:

	// PPoseOutput Output;
	// pose clips

	// play interval -> play all clips in the interval, passing output or even track itself into them.
	//???need base class for all clips? may help in editor and for common calculations like clips in interval, rel. time etc.
};

}
