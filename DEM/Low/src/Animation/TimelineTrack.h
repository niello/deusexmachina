#pragma once
#include <Core/Object.h>

// Base track for any timeline-playable object (e.g. animation clip, sound, event track etc).
// Track prototype can be stored as a resource. You must clone it and bind to outputs in order to play.

namespace DEM::Anim
{
using PTimelineTrack = Ptr<class CTimelineTrack>;

class CTimelineTrack : public ::Core::CObject
{
	RTTI_CLASS_DECL(DEM::Anim::CTimelineTrack, ::Core::CObject);

public:

	//???clips here? can make templated by clip type!
	// NB: whole timeline looping must be handled in a timeline player

	virtual PTimelineTrack Clone() const = 0;
	virtual float          GetDuration() const = 0;
	virtual void           PlayInterval(float PrevTime, float CurrTime, bool IsLast) = 0;

	// sample for time (or set time, if instanced)
	// rewinding, skipping etc, with intermediate effects (skip or execute script, skip VFX, set anim frame etc)

	//???blending, crossfading - where?

	//state: start, stop, is playing etc

	// duration overriding - default = from source, but can force abs time or speed rate (rel time)
	// timeline cursor will honor it, so track length on the timeline will change
	//!!!also need offset (start time)

	//!!!handle looping, looped track can have number of iterations or be infinite.
	//all timeline can be fixed or infinite too!

	//no keyframes on this level

	//!!!one of impls - nested timeline! composite track.
};

}
