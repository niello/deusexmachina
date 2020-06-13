#pragma once
#include <StdDEM.h>

// Base class for all player->world and character->world interactions.
// Each interaction accepts one or more targets, some of which may be optional.
// Interaction knows when it is available and owns interaction logic.

namespace DEM::Game
{
struct CTargetInfo;
class ITargetFilter; //ITargetType?

class IInteraction
{
public:

	// target: filter object, is optional, target slot count for this record
	// can mix mandatory and optional - some optional targets may require subsequent mandatory input!
	//???what if first target is optional? ignore? always set mandatory on load w/warning?

	// Name, Icon, Cursor, Desc?

	virtual U32  GetMaxTargetCount() const = 0;

	virtual bool IsTargetValid(U32 SlotIndex, const CTargetInfo& Target) const = 0;

	virtual bool Execute() const = 0;
};

}
