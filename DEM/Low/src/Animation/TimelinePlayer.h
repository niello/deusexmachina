#pragma once
#include <StdDEM.h>

// Plays a set of one or more timeline tracks.
// Can play the whole timeline (from the start to the last clip end) or a selected interval.
// Internal player time is zero based and monotonically increasing. It is mapped to the
// selected timeline interval with looping support. Selected interval can be explicitly
// set behind the last clip end.

namespace DEM::Anim
{
using PTimelinePlayer = std::unique_ptr<class CTimelinePlayer>;
class ITimelineTrack;

class CTimelinePlayer final
{
protected:

	ITimelineTrack* _pTrack = nullptr; //???refcounted strong?

	U32   _RemainingLoopCount = 0;

	float _StartTime = 0.f; // Timeline interval start
	float _EndTime = 0.f; // Timeline interval end. Must be strictly greater than _StartTime.
	float _PrevTime = 0.f;
	float _CurrTime = 0.f;
	float _Speed = 1.f;
	bool  _Paused = true; // Could use _Speed = 0, but Resume() requires us to preserve speed value

	void PlayInterval();
	bool OnLoopEnd();

public:

	static inline constexpr U32 LOOP_INFINITELY = 0; // For _RemainingLoopCount

	void  SetTrack(ITimelineTrack* pTrack);
	void  SetStartTime(float Time);
	void  SetEndTime(float Time);
	void  SetSpeed(float Speed) { _Speed = Speed; }
	void  SetPaused(bool Paused) { _Paused = Paused; }

	bool  Play(U32 LoopCount = 1) { _RemainingLoopCount = LoopCount; _Paused = !_pTrack; return !_Paused; }
	void  Stop() { /*_RemainingLoopCount = 0;*/ _Paused = true; SetTime(0.f); }
	void  SetTime(float Time) { _PrevTime = Time; _CurrTime = Time; }
	void  Rewind(float Time) { _CurrTime = Time; PlayInterval(); } //???what with _RemainingLoopCount? Set to 1?

	void  Update(float dt) { if (!_Paused) { _CurrTime += dt * _Speed; PlayInterval(); } }

	float GetSpeed() const { return _Speed; }
	U32   GetRemainingLoopCount() const { return _RemainingLoopCount; }
	bool  IsPlaying() const { return !_Paused; }
};

}
