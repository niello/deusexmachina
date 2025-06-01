#pragma once
#include <Core/Object.h>
#include <Data/StringID.h>

// Base track for any timeline-playable object (e.g. animation clip, sound, event track etc).
// Track prototype can be stored as a resource. You must clone it and bind to outputs in order to play.

namespace DEM::Anim
{
using PTimelineTrack = Ptr<class CTimelineTrack>;

class CTimelineTrack : public DEM::Core::CObject
{
	RTTI_CLASS_DECL(DEM::Anim::CTimelineTrack, DEM::Core::CObject);

protected:

	CStrID _Name; // Also used for connecting outputs

public:

	CTimelineTrack(CStrID Name = CStrID::Empty) : _Name(Name) {}

	//???clips here? can make templated by clip type!
	// NB: whole timeline looping must be handled in a timeline player

	virtual PTimelineTrack Clone() const = 0;
	virtual float          GetDuration() const = 0;
	virtual void           PlayInterval(float PrevTime, float CurrTime, bool IsLast) = 0;
	virtual void           Visit(std::function<void(CTimelineTrack&)>&& Visitor) { Visitor(*this); }
	virtual void           Visit(std::function<void(const CTimelineTrack&)>&& Visitor) const { Visitor(*this); }

	CStrID                 GetName() const { return _Name; }

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
