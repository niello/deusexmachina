#include "EventClip.h"
#include <Events/EventOutput.h>

namespace DEM::Anim
{

void CEventClip::AddEvent(float Time, CStrID ID, Data::CData Data)
{
	auto ItWhere = std::upper_bound(_Events.begin(), _Events.end(), Time, [](float EventTime, const CAnimEvent& Ev) { return EventTime < Ev.Time; });
	_Events.insert(ItWhere, CAnimEvent{ Time, ID, std::move(Data) });
}
//---------------------------------------------------------------------

void CEventClip::PlayInterval(float PrevTime, float CurrTime, bool /*IsLast*/, Events::IEventOutput& Output)
{
	// Prevent repeated triggering of an event under the playback cursor
	if (PrevTime == CurrTime) return;

	// Find a range of events to trigger, (PrevTime .. CurrTime]
	// NB: PrevTime is exclusive because it was fired at the previous PlayInterval call
	//???FIXME: how to trigger an event at the start time of the playback?! e.g. exactly at 0.f when playing from the beginning of the clip.
	if (PrevTime < CurrTime)
	{
		// Forward playback
		auto It = std::upper_bound(_Events.cbegin(), _Events.cend(), PrevTime, [](float Time, const CAnimEvent& Clip) { return Time < Clip.Time; });
		for (; It != _Events.cend() && It->Time <= CurrTime; ++It)
			Output.OnEvent(It->ID, It->Data, CurrTime - It->Time); // TODO: need global time diff, not a clip-local, because of looping
	}
	else
	{
		// Backward playback
		auto It = std::upper_bound(_Events.cbegin(), _Events.cend(), PrevTime, [](float Time, const CAnimEvent& Clip) { return Time > Clip.Time; });
		for (auto RIt = std::make_reverse_iterator(It); RIt != _Events.crend() && RIt->Time >= CurrTime; ++It)
			Output.OnEvent(RIt->ID, RIt->Data, CurrTime - RIt->Time);
	}
}
//---------------------------------------------------------------------

}
