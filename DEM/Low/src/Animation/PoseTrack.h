#pragma once
#include <Animation/TimelineTrack.h>

// Track that outputs local SRT transform to an abstract pose output.
// This is the most common track type, it is used for skeletal animation.

namespace DEM::Anim
{
using PPoseOutput = std::unique_ptr<class IPoseOutput>;
using PPoseClipBase = std::unique_ptr<class CPoseClipBase>;

class CPoseTrack : public ITimelineTrack
{
protected:

	PPoseOutput                _Output;
	std::vector<PPoseClipBase> _Clips; // start time? duration, if overridden (zero if as in anim. clip)? inside or here?
	//???!!!sort clips by start time?!

public:

	CPoseTrack();
	~CPoseTrack();

	virtual void PlayInterval(float PrevTime, float CurrTime) override;

	// pose track initialization - map each clip to output, where possible. Even LastPose can be mapped in advance.
	// recurse through blenders, handle mapped outputs for leaf sources (like in loading now).
	// when output changes, need to map all our clips to it once
};

}
