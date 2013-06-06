#pragma once
#ifndef __DEM_L1_COLLISION_OBJECT_STATIC_H__
#define __DEM_L1_COLLISION_OBJECT_STATIC_H__

#include <Physics/PhysicsObj.h>

// Static collision object can't move, transform changes are discrete and manual.
// Use this type of objects to represent static environment.

namespace Physics
{

class CCollisionObjStatic: public CPhysicsObj
{
	__DeclareClassNoFactory;

protected:

	bool			InternalInit();

public:

	virtual bool	Init(const Data::CParams& Desc, const vector3& Offset = vector3::Zero);
	bool			Init(CCollisionShape& CollShape, ushort CollGroup, ushort CollMask, const vector3& Offset = vector3::Zero);
	virtual bool	AttachToLevel(CPhysicsWorld& World);
	virtual void	RemoveFromLevel();
};

typedef Ptr<CCollisionObjStatic> PCollisionObjStatic;

}

#endif
