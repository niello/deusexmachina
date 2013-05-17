#pragma once
#ifndef __DEM_L2_PHYS_EXPL_AREA_IMPULSE_H__ //!!!to L1!
#define __DEM_L2_PHYS_EXPL_AREA_IMPULSE_H__

#include "AreaImpulse.h"
#include <Physics/Entity.h>

// Implements an area Impulse for a typical explosion. Applies an Impulse
// with exponentail falloff to all rigid bodies within the range of the
// explosion witch satisfy a line-of-sight test. After Apply() is called,
// the object can be asked about all physics entities which have been
// affected.

namespace Physics
{
class CContactPoint;
class CRigidBody;

class CExplosionAreaImpulse: public CAreaImpulse
{
	__DeclareClass(CExplosionAreaImpulse);

private:

	static nArray<CContactPoint> CollideContacts;

	bool HandleRigidBody(CRigidBody* pBody, const vector3& Position);

public:
	
	vector3	Position;
	float	Radius;
	float	Impulse;

	CExplosionAreaImpulse(): Radius(1.0f), Impulse(1.0f) {}
	virtual ~CExplosionAreaImpulse() {}

	void Apply();
};

}

#endif
