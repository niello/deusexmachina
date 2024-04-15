#pragma once
#include <Animation/Timeline/TimelineTrack.h>
#include <Data/Ptr.h>

// Track that outputs local SRT transform to an abstract pose output.
// This is the most common track type, it is used for skeletal animation.

// TODO: when clip time changed, need to sort / reinsert

namespace DEM::Anim
{
using PPoseOutput = std::unique_ptr<class IPoseOutput>;
using PPoseClipBase = std::unique_ptr<class CPoseClipBase>;
using PSkeletonInfo = Ptr<class CSkeletonInfo>;
using PPoseTrack = Ptr<class CPoseTrack>;

class CPoseTrack : public CTimelineTrack
{
protected:

	struct CClip
	{
		PPoseClipBase Clip;
		float         StartTime; // Must be less than next clip start time
		float         EndTime;   // Must not overlap with the next clip or at least must be less than its end time
	};

	PPoseOutput        _Output;
	PSkeletonInfo      _SkeletonInfo;
	std::vector<CClip> _Clips; // Sorted by start time ascending

public:

	CPoseTrack(CStrID Name);
	~CPoseTrack();

	void                   AddClip(PPoseClipBase&& Clip, float StartTime, float Duration /*, overlap resolve mode*/);
	void                   RefreshSkeletonInfo();
	void                   SetOutput(PPoseOutput&& Output);
	IPoseOutput*           GetOutput() const { return _Output.get(); }
	CSkeletonInfo*         GetSkeletonInfo() const { return _SkeletonInfo; } // non-const to create intrusive strong refs

	virtual PTimelineTrack Clone() const override;
	virtual float          GetDuration() const override;
	virtual void           PlayInterval(float PrevTime, float CurrTime, bool IsLast) override;
};

}
