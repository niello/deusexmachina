#pragma once
#ifndef __DEM_L2_PHYSICS_UTIL_H__ //!!!To L1!
#define __DEM_L2_PHYSICS_UTIL_H__

#include "kernel/ntypes.h"
#include "mathlib/vector.h"
#include <Physics/FilterSet.h>

// Implements some static physics utility methods.

namespace Physics
{
class CContactPoint;

class CPhysicsUtil //???need class? mb static funcs in namespace?
{
public:

	static bool RayCheck(const vector3& From, const vector3& To, CContactPoint& OutContact,
		const CFilterSet* ExcludeSet = NULL);
	static bool RayBundleCheck(const vector3& From, const vector3& To, const vector3& Up,
		const vector3& Left, float Radius, float& OutContactDist, const CFilterSet* ExcludeSet = NULL);
};

}
#endif
