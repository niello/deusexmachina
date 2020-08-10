#include "TimelinePlayer.h"
#include <Animation/TimelineTrack.h>

namespace DEM::Anim
{

//???what is the best way to handle reversed playback?
void CTimelinePlayer::PlayInterval(float PrevTime, float CurrTime)
{
	// Empty timeline, nothing to play
	if (!_pTrack || _EndTime <= _StartTime) return;

	const float Duration = _EndTime - _StartTime;

	// Convert previous time from linear player scale to timeline scale
	const float PrevLoopIndex = std::trunc(PrevTime / Duration);
	const float PrevLoopStartTime = PrevLoopIndex * Duration;

	float SegmentStartTimeTL;
	if (CurrTime > PrevTime)
	{
		// Play forward
		SegmentStartTimeTL = _StartTime + PrevTime - PrevLoopStartTime;
		float NextLoopStartTime = PrevLoopStartTime + Duration;
		while (NextLoopStartTime < CurrTime)
		{
			_pTrack->PlayInterval(SegmentStartTimeTL, _EndTime, false);
			SegmentStartTimeTL = _StartTime;
			NextLoopStartTime += Duration;
		}
	}
	else
	{
		// Play backward
		SegmentStartTimeTL = _StartTime + PrevTime - PrevLoopStartTime; //calc from end?
		/*float NextLoopStartTime = PrevLoopStartTime + Duration;
		while (NextLoopStartTime < CurrTime)
		{
			_pTrack->PlayInterval(SegmentStartTimeTL, _StartTime, false);
			SegmentStartTimeTL = _EndTime;
			NextLoopStartTime += Duration;
		}*/
	}

	// Convert current time from linear player scale to timeline scale
	const float CurrLoopIndex = std::trunc(CurrTime / Duration);
	const float CurrLoopStartTime = CurrLoopIndex * Duration;
	const float CurrTimeTL = _StartTime + CurrTime - CurrLoopStartTime;

	// The last segment finishes exactly at the current time
	_pTrack->PlayInterval(SegmentStartTimeTL, CurrTimeTL, true);
}
//---------------------------------------------------------------------

}
