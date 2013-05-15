#pragma once
#ifndef __DEM_L2_PHYSICS_SPHERE_SHAPE_H__ //!!!to L1!
#define __DEM_L2_PHYSICS_SPHERE_SHAPE_H__

#include "Shape.h"

// Spherical collision shape

namespace Physics
{
class CSphereShape: public CShape
{
	__DeclareClass(CSphereShape);

private:

	float Radius;

public:

	CSphereShape(): CShape(Sphere), Radius(1.0f) {}
	virtual ~CSphereShape() {}

	virtual void	Init(Data::PParams Desc);
	virtual bool	Attach(dSpaceID SpaceID);
	virtual void	RenderDebug(const matrix44& ParentTfm);

	void			SetRadius(float R) { n_assert(!IsAttached()); Radius = R; }
	float			GetRadius() const { return Radius; }
};

__RegisterClassInFactory(CSphereShape);

}

#endif
