#pragma once
#include <Core/Object.h>
#include <AI/Perception/Perception.h>

// AI level is an abstract space, like scene or CPhysicsLevel, that contains stimuli,
// AI hints and other AI-related world info. Also AILevel is intended to serve as a
// navigation manager in the future.

namespace Debug
{
	class CDebugDraw;
}

namespace DEM::AI
{
using PAILevel = Ptr<class CAILevel>;

class CAILevel : public DEM::Core::CObject
{
protected:

	std::vector<CStimulusEvent> _StimulusEvents; // pending for processing in the next AI frame

public:

	//!!!global, not per level!
	//float _LowestUsefulIntensity[static_cast<size_t>(ESenseModality::Count)] = { 0.f };
	//void  SetLowestUsefulIntensity(ESenseModality Sense, float Value) { _LowestUsefulIntensity[static_cast<size_t>(Sense)] = Value; }
	//float GetLowestUsefulIntensity(ESenseModality Sense) const { return _LowestUsefulIntensity[static_cast<size_t>(Sense)]; }

	float GetStimulusMaskingAt(ESenseModality Sense, const rtm::vector4f& Pos) const;

	void  AddStimulusEvent(const CStimulusEvent& Event);

	template<typename F>
	void ProcessStimulusEvents(F Callback)
	{
		for (const auto& Event : _StimulusEvents)
			Callback(Event);
		_StimulusEvents.clear();
	}

	void  RenderDebug(Debug::CDebugDraw& DebugDraw);
};
//---------------------------------------------------------------------

}
