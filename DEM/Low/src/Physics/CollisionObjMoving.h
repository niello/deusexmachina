#pragma once
#ifndef __DEM_L1_COLLISION_OBJECT_MOVING_H__
#define __DEM_L1_COLLISION_OBJECT_MOVING_H__

#include <Physics/PhysicsObject.h>

// Moving collision object inherits transform from the scene node, collides with
// dynamic bodies as a moving object, but doesn't respond to collisions.
// Use this type of objects to represent objects controlled by an animation,
// by user input or any other non-physics controller.

namespace Physics
{

//???own motion state or inherit from it?
class CCollisionObjMoving: public CPhysicsObject
{
	RTTI_CLASS_DECL;

protected:

	bool			InternalInit();
	void			InternalTerm();
	virtual void	GetTransform(btTransform& Out) const;

public:

	virtual ~CCollisionObjMoving() { InternalTerm(); }

	virtual bool	Init(const Data::CParams& Desc, const vector3& Offset = vector3::Zero);
	bool			Init(CCollisionShape& CollShape, U16 CollGroup, U16 CollMask, const vector3& Offset = vector3::Zero);
	virtual void	Term();
	virtual bool	AttachToLevel(CPhysicsLevel& World);
	virtual void	RemoveFromLevel();

	virtual void	SetTransform(const matrix44& Tfm);
};

typedef Ptr<CCollisionObjMoving> PCollisionObjMoving;

}

#endif
