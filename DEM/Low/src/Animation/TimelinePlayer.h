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

class CTimelinePlayer
{
protected:

	ITimelineTrack* _pTrack = nullptr; //???refcounted strong?

	float _StartTime = 0.f; // Timeline interval start
	float _EndTime = 0.f; // Timeline interval end. Must be strictly greater than _StartTime.
	float _PrevTime = 0.f;
	float _CurrTime = 0.f;
	float _Speed = 1.f;

	//???float _StopTime? When stop playback. How to handle negative (reverse)?

	//???static? or don't pass args?
	void PlayInterval(float PrevTime, float CurrTime);

public:

	void SetTrack(ITimelineTrack* pTrack);
	void SetStartTime(float Time);
	void SetEndTime(float Time);
	void SetSpeed(float Speed);

	//!!!when set timeline track, must update _StartTime to zero and _EndTime to the last clip end!

	// start time (def 0), end time (def inf), loop count (0 - infinite), speed, negative for reverse
	//???start, end times - Play() args or player params?
	//???speed and loop count - Play() args? Not settings of the player itself? Anyway must remember them, but when to set?

	// Update(dt), tracks must be bound to outputs
	//!!!must fix too big times if has looping! can also stop after the last clip (if no current clip was found on any track)!

	// SetTime (move prev and curr), Rewind(dir?) (move curr only to play interval instead of skipping)

	// Play, Pause, IsPlaying etc

	//???how to handle multiple loops inside the same frame?

	/*

	bool  Play() { _Paused = !_Clip; return !_Paused; }
	void  Stop() { _Paused = true; _CurrTime = 0.f; }
	void  Pause() { _Paused = true; }
	void  SetSpeed(float Speed) { _Speed = Speed; }
	void  SetLooped(bool Loop) { _Loop = Loop; }
	float GetSpeed() const { return _Speed; }
	bool  IsLooped() const { return _Loop; }
	bool  IsPlaying() const { return !_Paused; }

	*/
};

}
