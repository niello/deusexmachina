#pragma once
#ifndef __DEM_L1_PHYSICS_OBJECT_H__
#define __DEM_L1_PHYSICS_OBJECT_H__

#include <Physics/CollisionShape.h>
#include <Math/Vector3.h>

// Base class for all physics objects. Instantiates shared shape and locates it in a physics world.

class CAABB;
class quaternion;
class btTransform;
class btCollisionObject;

namespace Data
{
	class CParams;
}

namespace Physics
{
class CPhysicsWorld;

class CPhysicsObject: public Core::CObject
{
	__DeclareClassNoFactory;

protected:

	PCollisionShape		Shape;
	vector3				ShapeOffset;	// Offset between a center of mass and a graphics
	ushort				Group;
	ushort				Mask;
	btCollisionObject*	pBtCollObj;
	CPhysicsWorld*		pWorld;
	void*				pUserPtr;

	bool				Init(CCollisionShape& CollShape, ushort CollGroup, ushort CollMask, const vector3& Offset = vector3::Zero);
	void				InternalTerm();
	virtual void		GetTransform(btTransform& Out) const;

public:

	CPhysicsObject(): pBtCollObj(NULL), pWorld(NULL), pUserPtr(NULL) {}
	virtual ~CPhysicsObject() { InternalTerm(); }

	virtual bool		Init(const Data::CParams& Desc, const vector3& Offset = vector3::Zero);
	virtual void		Term();
	virtual bool		AttachToLevel(CPhysicsWorld& World);
	virtual void		RemoveFromLevel();
	bool				IsInitialized() const { return Shape.IsValidPtr() && pBtCollObj; }
	bool				IsAttachedToLevel() const { return !!pWorld; }

	virtual void		SetTransform(const matrix44& Tfm);
	virtual void		GetTransform(vector3& OutPos, quaternion& OutRot) const;
	void				SetUserData(void* pPtr) { pUserPtr = pPtr; }
	void*				GetUserData() const { return pUserPtr; }
	void				GetGlobalAABB(CAABB& OutBox) const;
	void				GetPhysicsAABB(CAABB& OutBox) const;
	btCollisionObject*	GetBtObject() const { return pBtCollObj; }
	CPhysicsWorld*		GetWorld() const { return pWorld; }
	ushort				GetCollisionGroup() const { return Group; }
	ushort				GetCollisionMask() const { return Mask; }
	const vector3&		GetShapeOffset() const { return ShapeOffset; }
};

typedef Ptr<CPhysicsObject> PPhysicsObj;

}

#endif
