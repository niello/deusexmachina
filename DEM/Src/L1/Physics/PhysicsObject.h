#pragma once
#ifndef __DEM_L1_PHYSICS_OBJECT_H__
#define __DEM_L1_PHYSICS_OBJECT_H__

#include <Physics/CollisionShape.h>
#include <Math/Vector3.h>

// Base class for all physics objects. Instantiates shared shape and locates it in a physics world.

class CAABB;
class quaternion;
class matrix44;
class btTransform;
class btCollisionObject;

namespace Data
{
	class CParams;
}

namespace Physics
{
class CPhysicsLevel;

class CPhysicsObject: public Core::CObject
{
	__DeclareClassNoFactory;

protected:

	PCollisionShape		Shape;
	vector3				ShapeOffset;	// Offset between a center of mass and a graphics
	U16					Group;
	U16					Mask;
	btCollisionObject*	pBtCollObj;
	CPhysicsLevel*		pWorld;
	void*				pUserPtr;

	bool				Init(CCollisionShape& CollShape, U16 CollGroup, U16 CollMask, const vector3& Offset = vector3::Zero);
	void				InternalTerm();
	virtual void		GetTransform(btTransform& Out) const;

public:

	CPhysicsObject(): pBtCollObj(NULL), pWorld(NULL), pUserPtr(NULL) {}
	virtual ~CPhysicsObject() { InternalTerm(); }

	virtual bool		Init(const Data::CParams& Desc, const vector3& Offset = vector3::Zero);
	virtual void		Term();
	virtual bool		AttachToLevel(CPhysicsLevel& World);
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
	CPhysicsLevel*		GetWorld() const { return pWorld; }
	U16					GetCollisionGroup() const { return Group; }
	U16					GetCollisionMask() const { return Mask; }
	const vector3&		GetShapeOffset() const { return ShapeOffset; }
};

typedef Ptr<CPhysicsObject> PPhysicsObj;

}

#endif
