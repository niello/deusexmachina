#pragma once
#ifndef __DEM_L1_TIME_SOURCE_H__
#define __DEM_L1_TIME_SOURCE_H__

#include <Core/Object.h>

// A generic time source object which is attached to the CTimeServer.
// Each time source tracks its own time independently from the other
// time sources, they can also be paused and unpaused independently from
// each other, and they may also run faster or slower then real time.

namespace Time
{

class CTimeSource: public Core::CObject
{
	__DeclareClassNoFactory;

protected:

	friend class CTimeServer; //!!!For loading, revisit

	float	Time;
	float	FrameTime;
	float	TimeFactor;
	UPTR	FrameID;
	int		PauseCounter;

public:

	CTimeSource();
	virtual ~CTimeSource() {}

	void	Update(float _FrameTime);
	void	Reset() { Time = 0.0; FrameTime = 0.0; }
	void	Pause() { ++PauseCounter; }
	void	Unpause() { if (PauseCounter > 0) --PauseCounter; }
	void	ForceUnpause() { PauseCounter = 0; }
	bool	IsPaused() const { return (PauseCounter > 0); }
	int		GetPauseCount() const { return PauseCounter; }

	void	SetFactor(float Factor) { n_assert(Factor > 0.f); TimeFactor = Factor; }
	float	GetFactor() const { return TimeFactor; }
	float	GetTime() const { return Time; }
	float	GetFrameTime() const { return IsPaused() ? 0.f : FrameTime; }
	UPTR	GetFrameID() const { return FrameID; }
};

typedef Ptr<CTimeSource> PTimeSource;

inline CTimeSource::CTimeSource():
	FrameTime(0.001f),
	Time(0.0),
	PauseCounter(0),
	TimeFactor(1.0f),
	FrameID(0)
{
}
//---------------------------------------------------------------------

}

#endif