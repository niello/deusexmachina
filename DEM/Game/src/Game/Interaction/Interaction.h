#pragma once
#include <StdDEM.h>

// 

namespace DEM::Game
{
class CTarget;

class IInteraction
{
public:

	// can add factory methods to abilities and actions to create from CParams etc

	// list of different targets
	// list of target slots? or instead in the first list store number of slots? is enough?
	// number of mandatory targets, other slots will be optional

	// A, A, A, B, A, B vs A(4), B(2), easier, but less control over the order
	// but still can duplicate descs and write A(3), B(1), A(1), B(1). Not really oftenly required?

	virtual UPTR GetMaxTargetCount() const = 0;

	//!!!PTarget!
	virtual CTarget* CreateTarget(UPTR Index) const = 0;

	virtual bool Execute() const = 0;
};

}