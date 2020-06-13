#pragma once
#include <StdDEM.h>

// Base class for all player->world and character->world interactions.
// Each interaction accepts one or more targets, some of which may be optional.
// Interaction knows when it is available and owns interaction logic.

namespace DEM::Game
{
struct CTargetInfo;
class CTarget;

class IInteraction
{
public:

	// target: filter object, is mandatory, target count

	// list of different targets
	// list of target slots? or instead in the first list store number of slots? is enough?
	// number of mandatory targets, other slots will be optional

	// A, A, A, B, A, B vs A(4), B(2), easier, but less control over the order
	// but still can duplicate descs and write A(3), B(1), A(1), B(1). Not really oftenly required?

	virtual U32  GetMaxTargetCount() const = 0;

	virtual bool IsTargetValid(U32 SlotIndex, const CTargetInfo& Target) const = 0;

	virtual bool Execute() const = 0;
};

}
