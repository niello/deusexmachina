#pragma once
#ifndef __DEM_L2_PHYSICS_AREA_IMPULSE_H__ //!!!to L1!
#define __DEM_L2_PHYSICS_AREA_IMPULSE_H__

#include <Core/RefCounted.h>

// General base class for area impulses. An area impulse applies an impulse
// to all objects within a given area volume. Subclasses implement specific
// volumes and behaviors. Most useful for explosions and similar stuff.

namespace Physics
{

class CAreaImpulse: public Core::CRefCounted
{
	DeclareRTTI;

public:

	virtual ~CAreaImpulse() = 0;

	virtual void Apply() = 0;
};

typedef Ptr<CAreaImpulse> PAreaImpulse;

}

#endif
