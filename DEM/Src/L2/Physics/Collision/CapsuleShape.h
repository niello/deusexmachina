#pragma once
#ifndef __DEM_L2_PHYSICS_CAPSULE_SHAPE_H__ //!!!to L1!
#define __DEM_L2_PHYSICS_CAPSULE_SHAPE_H__

#include "Shape.h"

// Capsule collision shape

namespace Physics
{

class CCapsuleShape: public CShape
{
	DeclareRTTI;
	DeclareFactory(CCapsuleShape);

private:

	float Radius;
	float Length;	// Not counting caps height

public:

	CCapsuleShape(): CShape(Capsule), Radius(1.0f), Length(1.0f) {}
	virtual ~CCapsuleShape() {}

	virtual void	Init(Data::PParams Desc);
	virtual bool	Attach(dSpaceID SpaceID);
	virtual void	RenderDebug(const matrix44& ParentTfm);

	void			SetRadius(float R) { Radius = R; }
	float			GetRadius() const { return Radius; }
	void			SetLength(float Len) { Length = Len; }
	float			GetLength() const { return Length; }
};

RegisterFactory(CCapsuleShape);

}

#endif
