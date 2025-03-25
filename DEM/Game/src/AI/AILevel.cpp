#include "AILevel.h"
#include <Debug/DebugDraw.h>

namespace DEM::AI
{

float CAILevel::GetStimulusMaskingAt(ESenseModality Sense, const rtm::vector4f& Pos) const
{
	// TODO: ambient noise masks sounds, wind affects smells etc
	return 0.f;
}
//---------------------------------------------------------------------

void CAILevel::AddStimulusEvent(const CStimulusEvent& Event)
{
	if (Event.Intensity <= 0.f) return;

	_StimulusEvents.push_back(Event);
	n_assert_dbg(_StimulusEvents.size() < 5000);
}
//---------------------------------------------------------------------

void CAILevel::RenderDebug(Debug::CDebugDraw& DebugDraw)
{
	NOT_IMPLEMENTED;
}
//---------------------------------------------------------------------

}
