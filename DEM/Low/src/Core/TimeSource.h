#pragma once
#include <Core/Object.h>

// A generic time source object which is attached to the CCoreServer.
// Each time source tracks its own time independently from the other
// time sources, they can also be paused and unpaused independently from
// each other, and they may also run faster or slower then real time.

namespace Core
{

class CTimeSource: public CObject
{
	RTTI_CLASS_DECL(Core::CTimeSource, Core::CObject);

protected:

	friend class CCoreServer; //!!!For loading, revisit

	float	Time = 0.f;
	float	FrameTime = 0.001f;
	float	TimeFactor = 1.0f;
	UPTR	FrameID = 0;
	int		PauseCounter = 0;

public:

	void	Update(float _FrameTime);
	void	Reset() { Time = 0.f; FrameTime = 0.f; }
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

}
