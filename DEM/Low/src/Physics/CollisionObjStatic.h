#pragma once
#ifndef __DEM_L1_COLLISION_OBJECT_STATIC_H__
#define __DEM_L1_COLLISION_OBJECT_STATIC_H__

#include <Physics/PhysicsObject.h>

// Static collision object can't move, transform changes are discrete and manual.
// Use this type of objects to represent static environment.

namespace Physics
{

class CCollisionObjStatic: public CPhysicsObject
{
	RTTI_CLASS_DECL;

protected:

	bool			InternalInit();

public:

	virtual bool	Init(const Data::CParams& Desc, const vector3& Offset = vector3::Zero);
	bool			Init(CCollisionShape& CollShape, U16 CollGroup, U16 CollMask, const vector3& Offset = vector3::Zero);
	virtual bool	AttachToLevel(CPhysicsLevel& World);
	virtual void	RemoveFromLevel();
};

typedef Ptr<CCollisionObjStatic> PCollisionObjStatic;

}

#endif
