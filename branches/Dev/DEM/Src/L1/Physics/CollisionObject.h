#pragma once
#ifndef __DEM_L1_COLLISION_OBJECT_H__
#define __DEM_L1_COLLISION_OBJECT_H__

#include <Physics/CollisionShape.h>

// Collision object instantiates shared shape and locates it in a physics world

class btCollisionObject;

namespace Physics
{

class CCollisionObject: public Core::CRefCounted
{
	__DeclareClassNoFactory;

protected:

	PCollisionShape		Shape;
	btCollisionObject*	pBtCollObj;

public:

	CCollisionObject(CCollisionShape& CollShape);
	~CCollisionObject();

	btCollisionObject*	GetBtObject() const { return pBtCollObj; }
};

typedef Ptr<CCollisionObject> PCollisionObject;

}

#endif
