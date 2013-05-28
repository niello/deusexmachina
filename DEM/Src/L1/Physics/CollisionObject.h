#pragma once
#ifndef __DEM_L1_COLLISION_OBJECT_H__
#define __DEM_L1_COLLISION_OBJECT_H__

#include <Physics/CollisionShape.h>

// Collision object instantiates shared shape and locates it in a physics world

class btCollisionObject;

namespace Physics
{
class CPhysicsWorld;

class CCollisionObject: public Core::CRefCounted
{
	__DeclareClassNoFactory;

protected:

	PCollisionShape		Shape;
	btCollisionObject*	pBtCollObj;
	CPhysicsWorld*		pWorld;

public:

	CCollisionObject(CCollisionShape& CollShape);
	~CCollisionObject();

	void				OnAdd(CPhysicsWorld& PhysWorld) { n_assert(!pWorld); pWorld = &PhysWorld; }
	void				OnRemove() { n_assert(pWorld); pWorld = NULL; }

	bool				SetTransform(const matrix44& Tfm);
	btCollisionObject*	GetBtObject() const { return pBtCollObj; }
};

typedef Ptr<CCollisionObject> PCollisionObject;

}

#endif
