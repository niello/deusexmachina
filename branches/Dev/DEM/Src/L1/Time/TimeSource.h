#pragma once
#ifndef __DEM_L1_TIME_SOURCE_H__
#define __DEM_L1_TIME_SOURCE_H__

#include <Core/RefCounted.h>
#include <StdDEM.h>

// A generic time source object which is attached to the CTimeServer.
// Each time source tracks its own time independently from the other
// time sources, they can also be paused and unpaused independently from
// each other, and they may also run faster or slower then real time.
//
// Based on mangalore TimeSource_(C) 2006 Radon Labs GmbH

namespace Time
{

class CTimeSource: public Core::CRefCounted //???need?
{
	DeclareRTTI;
	DeclareFactory(CTimeSource);

protected:

	nTime	Time;
	nTime	FrameTime;
	float	TimeFactor;
	DWORD	FrameID;
	int		PauseCounter;

public:

	CTimeSource();
	virtual ~CTimeSource() {}

	void	Update(nTime _FrameTime);
	void	Reset() { Time = 0.0; FrameTime = 0.0; }
	void	Pause() { ++PauseCounter; }
	void	Unpause() { if (PauseCounter > 0) --PauseCounter; }
	void	ForceUnpause() { PauseCounter = 0; }
	bool	IsPaused() const { return (PauseCounter > 0); }

	void	SetFactor(float Factor) { n_assert(Factor > 0.f); TimeFactor = Factor; }
	float	GetFactor() const { return TimeFactor; }
	nTime	GetTime() const { return Time; }
	nTime	GetFrameTime() const { return IsPaused() ? 0.f : FrameTime; }
	DWORD	GetFrameID() const { return FrameID; }
};

typedef Ptr<CTimeSource> PTimeSource;

}

#endif