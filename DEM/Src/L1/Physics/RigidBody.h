#pragma once
#ifndef __DEM_L1_RIGID_BODY_H__
#define __DEM_L1_RIGID_BODY_H__

#include <Physics/PhysicsObject.h>

// Rigid body is a center of mass that has shape and transform. Rigid body is
// simulated by physics world and can be used as transformation source.

class btRigidBody;

namespace Physics
{

class CRigidBody: public CPhysicsObject
{
	__DeclareClassNoFactory;

protected:

	float Mass;

	bool			InternalInit(float BodyMass);
	void			InternalTerm();
	virtual void	GetTransform(btTransform& Out) const;

public:

	virtual ~CRigidBody() { InternalTerm(); }

	virtual bool	Init(const Data::CParams& Desc, const vector3& Offset = vector3::Zero);
	bool			Init(CCollisionShape& CollShape, float BodyMass, U16 CollGroup, U16 CollMask, const vector3& Offset = vector3::Zero);
	virtual void	Term();
	virtual bool	AttachToLevel(CPhysicsLevel& World);
	virtual void	RemoveFromLevel();

	virtual void	SetTransform(const matrix44& Tfm);
	virtual void	GetTransform(vector3& OutPos, quaternion& OutRot) const;
	void			SetTransformChanged(bool Changed);
	bool			IsTransformChanged() const;
	float			GetInvMass() const;
	float			GetMass() const { return Mass; }
	bool			IsActive() const;
	bool			IsAlwaysActive() const;
	bool			IsAlwaysInactive() const;
	void			MakeActive();
	void			MakeAlwaysActive();
	void			MakeAlwaysInactive();
	btRigidBody*	GetBtBody() const { return (btRigidBody*)pBtCollObj; }
};

typedef Ptr<CRigidBody> PRigidBody;

}

#endif
