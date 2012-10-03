#pragma once
#ifndef __DEM_L1_INPUT_CONTROL_LAYOUT_H__
#define __DEM_L1_INPUT_CONTROL_LAYOUT_H__

#include <Input/InputMappingEvent.h>
#include <Input/InputMappingState.h>

// Control layout is a set of mappings, each of which translates input
// into abstract events and states.

namespace Input
{

class CControlLayout: public Core::CRefCounted
{
private:

	bool						Enabled;
	nArray<CInputMappingEvent>	EventMappings;
	nArray<CInputMappingState>	StateMappings;

	//???listen ResetInput?

public:

	CControlLayout(): Enabled(false) {}

	bool Init(const Data::CParams& Desc);
	void Enable();
	void Disable();
	bool IsEnabled() const { return Enabled; }
	void Reset();	// Reset all states

	//!!!need R/O access to states to check them!
};

typedef Ptr<CControlLayout> PControlLayout;

}

#endif
