#pragma once
#include <Events/EventHandler.h>

// Plays a Flow asset and tracks its state.
// Only one action can be active at the same time.

namespace DEM::Flow
{

class CFlowPlayer
{
private:

	// Asset to play
	// Active action
	// Active action transparent data object, if created (tmp values and event subs are stored there). Cache to avoid reallocations on looped actions?
	// Active action state (active/update, wait, output pin, finished without valid output pin, error)
	// Variable set

public:

	// Update(float dt)
	//??? put to sleep / resume, for the case when the current action is not updateable, e.g. waits an external signal
};

}
