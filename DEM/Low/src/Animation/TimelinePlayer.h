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

	float _StartTime = 0.f; // Timeline interval start
	float _EndTime = 0.f; // Timeline interval end. Must be strictly greater than _StartTime.
	float _PrevTime = 0.f;
	float _CurrTime = 0.f;
	float _Speed = 1.f;

	//???static? or don't pass args?
	void PlayInterval(float PrevTime, float CurrTime);

public:

	void SetTrack(ITimelineTrack* pTrack);
	void SetStartTime(float Time);
	void SetEndTime(float Time);
	void SetSpeed(float Speed) { _Speed = Speed; }

	void Update(float dt) { _CurrTime += dt; PlayInterval(_PrevTime, _CurrTime); }

	// Need play, pause, speed, reverse, loop and loop limit
	//???float _StopTime? When stop playback. How to handle negative (reverse)?
	// SetTime (move prev and curr), Rewind(dir?) (move curr only to play interval instead of skipping)
	// Play, Pause, IsPlaying etc
	//!!!must fix too big times if has looping! can also stop after the last clip (if no current clip was found on any track)!

	/*
	bool  Play() { _Paused = !_Clip; return !_Paused; }
	void  Stop() { _Paused = true; _CurrTime = 0.f; }
	void  Pause() { _Paused = true; }
	void  SetLooped(bool Loop) { _Loop = Loop; }
	float GetSpeed() const { return _Speed; }
	bool  IsLooped() const { return _Loop; }
	bool  IsPlaying() const { return !_Paused; }
	*/
};

}
