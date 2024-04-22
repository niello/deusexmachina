#pragma once
#include <Data/Ptr.h>
#include <Data/StringID.h>
#include <Data/Data.h>

// A clip for an event track contains events to be dispatched during playback

namespace DEM::Events
{
	class IEventOutput;
}

namespace DEM::Anim
{
using PEventClip = std::unique_ptr<class CEventClip>;

class CEventClip
{
protected:

	struct CAnimEvent
	{
		float       Time;
		CStrID      ID;
		Data::CData Data;
	};

	std::vector<CAnimEvent> _Events; // Sorted by Time ascending

public:

	void AddEvent(float Time, CStrID ID, Data::CData Data = {});
	void PlayInterval(float PrevTime, float CurrTime, bool IsLast, Events::IEventOutput& Output);
};

}
