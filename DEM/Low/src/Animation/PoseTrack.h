#pragma once
#include <Animation/TimelineTrack.h>
#include <Data/Ptr.h>

// Track that outputs local SRT transform to an abstract pose output.
// This is the most common track type, it is used for skeletal animation.

// TODO: when clip time changed, need to sort / reinsert

namespace DEM::Anim
{
using PPoseOutput = Ptr<class IPoseOutput>;
using PPoseClipBase = std::unique_ptr<class CPoseClipBase>;
using PPoseTrack = std::unique_ptr<class CPoseTrack>;

class CPoseTrack : public ITimelineTrack
{
protected:

	struct CClip
	{
		PPoseClipBase Clip;
		float         StartTime;
		float         EndTime;   // Must not overlap with the next clip or at least must be less than its end time
	};

	PPoseOutput        _Output;
	std::vector<CClip> _Clips; // Sorted by start time ascending

public:

	CPoseTrack();
	~CPoseTrack();

	void          SetOutput(PPoseOutput&& Output);
	void          AddClip(PPoseClipBase&& Clip, float StartTime, float Duration /*, overlap resolve mode*/);

	virtual float GetDuration() const override;
	virtual void  PlayInterval(float PrevTime, float CurrTime, bool IsLast) override;
};

}
