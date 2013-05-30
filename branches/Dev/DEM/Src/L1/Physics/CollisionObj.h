#pragma once
#ifndef __DEM_L1_COLLISION_OBJECT_H__
#define __DEM_L1_COLLISION_OBJECT_H__

#include <Physics/CollisionShape.h>

// Collision object instantiates shared shape and locates it in a physics world.

class bbox3;
class btCollisionObject;

namespace Physics
{
class CPhysicsWorld;

class CCollisionObj: public Core::CRefCounted
{
	__DeclareClassNoFactory;

protected:

	PCollisionShape		Shape;
	vector3				Offset;			// Offset between a center of mass and a graphics
	ushort				Group;
	ushort				Mask;
	btCollisionObject*	pBtCollObj;
	CPhysicsWorld*		pWorld;

public:

	CCollisionObj(): pBtCollObj(NULL), pWorld(NULL) {}
	virtual ~CCollisionObj() { Term(); }

	virtual bool		Init(CCollisionShape& CollShape, ushort Group, ushort Mask, const vector3& Offset);
	virtual void		Term();
	virtual bool		AttachToLevel(CPhysicsWorld& World) = 0;
	virtual void		RemoveFromLevel() = 0;

	bool				SetTransform(const matrix44& Tfm);
	void				GetGlobalAABB(bbox3& OutBox) const;
	btCollisionObject*	GetBtObject() const { return pBtCollObj; }
};

typedef Ptr<CCollisionObj> PCollisionObj;

}

#endif
