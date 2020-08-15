#include "TimelinePlayer.h"
#include <Animation/TimelineTrack.h>

namespace DEM::Anim
{

void CTimelinePlayer::SetTrack(ITimelineTrack* pTrack)
{
	_pTrack = pTrack;
	SetStartTime(0.f);
	SetEndTime(_pTrack ? _pTrack->GetDuration() : 0.f);
}
//---------------------------------------------------------------------

void CTimelinePlayer::SetStartTime(float Time)
{
	_StartTime = std::clamp(Time, 0.f, _EndTime);
}
//---------------------------------------------------------------------

void CTimelinePlayer::SetEndTime(float Time)
{
	_EndTime = std::max(Time, _StartTime);
}
//---------------------------------------------------------------------

//???what is the best way to handle reversed playback?
void CTimelinePlayer::PlayInterval(float PrevTime, float CurrTime)
{
	// Empty timeline, nothing to play
	if (!_pTrack || _EndTime <= _StartTime) return;

	const float Duration = _EndTime - _StartTime;

	// Convert previous time from linear player scale to timeline scale
	float LoopStartTime = std::floor(PrevTime / Duration) * Duration;
	float SegmentStartTimeTL = _StartTime + PrevTime - LoopStartTime;

	if (CurrTime > PrevTime)
	{
		// Play forward
		LoopStartTime += Duration;
		while (LoopStartTime < CurrTime)
		{
			_pTrack->PlayInterval(SegmentStartTimeTL, _EndTime, false);
			SegmentStartTimeTL = _StartTime;
			LoopStartTime += Duration;
		}
	}
	else
	{
		// Play backward
		while (LoopStartTime > CurrTime)
		{
			_pTrack->PlayInterval(SegmentStartTimeTL, _StartTime, false);
			SegmentStartTimeTL = _EndTime;
			LoopStartTime -= Duration;
		}
	}

	// Convert current time from linear player scale to timeline scale
	const float CurrLoopStartTime = std::floor(CurrTime / Duration) * Duration;
	const float CurrTimeTL = _StartTime + CurrTime - CurrLoopStartTime;

	// The last segment ends exactly at the current time
	_pTrack->PlayInterval(SegmentStartTimeTL, CurrTimeTL, true);
}
//---------------------------------------------------------------------

}
