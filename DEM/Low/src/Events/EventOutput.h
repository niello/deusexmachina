#pragma once
#include <Data/Data.h>

// Abstract output interface for incoming events. Used e.g. for firing or buffering animation events.

namespace DEM::Events
{

class IEventOutput
{
public:

	//!!!
	// IEventOutput can be immediate or buffering, event or signal. Signal can be templated with void or serializable argument for loading it from CData.
	// PlayInterval will provide real CurrTime and can provide desired time
	// also need to provide a weight from somewhere. Set to output?
	// can add fields like entity ID to output subclasses
	//???weight as arg? or set weight to output?
	virtual void OnEvent(CStrID ID, const Data::CData& Data, float TimeShift) = 0;
};

}
