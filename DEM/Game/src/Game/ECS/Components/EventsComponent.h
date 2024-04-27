#pragma once
#include <Events/EventOutputBuffer.h>
//#include <Data/Metadata.h>
//#include <deque>

// An arbitrary event collector and dispatcher for an entity. E.g. can be used to collect and fire animation events.

namespace DEM::Game
{

struct CEventsComponent
{
	Events::CEventOutputBuffer Buffer;
	Events::CSignal<void(DEM::Game::HEntity, CStrID, const Data::CParams*, float TimeOffset)> OnEvent;
};

}
