#include "TimelinePlayer.h"
#include <Animation/TimelineTrack.h>

namespace DEM::Anim
{
CTimelinePlayer::CTimelinePlayer() = default;
CTimelinePlayer::~CTimelinePlayer() = default;

void CTimelinePlayer::SetTrack(const PTimelineTrack& Track)
{
	_Track = Track;
	SetStartTime(0.f);
	SetEndTime(_Track ? _Track->GetDuration() : 0.f);
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

bool CTimelinePlayer::OnLoopEnd()
{
	if (_RemainingLoopCount)
	{
		--_RemainingLoopCount;
		if (!_RemainingLoopCount)
		{
			Stop();
			return true;
		}
	}

	return false;
}
//---------------------------------------------------------------------

//!!!must fix too big times if has looping! can also stop after the last clip (for all nested tracks too)
void CTimelinePlayer::PlayInterval()
{
	// Empty timeline, nothing to play
	if (!_Track || _EndTime <= _StartTime) return;

	const float PrevTime = _PrevTime;
	_PrevTime = _CurrTime;

	const float Duration = _EndTime - _StartTime;

	// Convert previous time from linear player scale to timeline scale
	float LoopStartTime = std::floor(PrevTime / Duration) * Duration;
	float SegmentStartTimeTL = _StartTime + PrevTime - LoopStartTime;

	if (_CurrTime > PrevTime)
	{
		// Play forward
		LoopStartTime += Duration;
		while (LoopStartTime < _CurrTime)
		{
			_Track->PlayInterval(SegmentStartTimeTL, _EndTime, (_RemainingLoopCount == 1));
			if (OnLoopEnd()) return;
			SegmentStartTimeTL = _StartTime;
			LoopStartTime += Duration;
		}
	}
	else
	{
		// Play backward
		while (LoopStartTime > _CurrTime)
		{
			_Track->PlayInterval(SegmentStartTimeTL, _StartTime, (_RemainingLoopCount == 1));
			if (OnLoopEnd()) return;
			SegmentStartTimeTL = _EndTime;
			LoopStartTime -= Duration;
		}
	}

	// Convert current time from linear player scale to timeline scale
	const float CurrLoopStartTime = std::floor(_CurrTime / Duration) * Duration;
	const float CurrTimeTL = _StartTime + _CurrTime - CurrLoopStartTime;

	// The last segment ends exactly at the current time
	_Track->PlayInterval(SegmentStartTimeTL, CurrTimeTL, true);

	// Process exact loop end
	if (LoopStartTime == _CurrTime && OnLoopEnd()) return;
}
//---------------------------------------------------------------------

}
