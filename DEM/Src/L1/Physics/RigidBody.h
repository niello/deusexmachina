#pragma once
#ifndef __DEM_L1_RIGID_BODY_H__
#define __DEM_L1_RIGID_BODY_H__

#include <Physics/PhysicsObj.h>

// Rigid body is a center of mass that has shape and transform. Rigid body is
// simulated by physics world and can be used as transformation source.

namespace Scene
{
	class CSceneNode;
}

namespace Physics
{

class CRigidBody: public CPhysicsObj
{
	__DeclareClassNoFactory;

protected:

	void			InternalTerm();
	virtual void	GetTransform(btTransform& Out) const;

public:

	virtual ~CRigidBody() { InternalTerm(); }

	virtual bool	Init(const Data::CParams& Desc, const vector3& Offset = vector3::Zero);
	virtual void	Term();
	virtual bool	AttachToLevel(CPhysicsWorld& World);
	virtual void	RemoveFromLevel();

	virtual void	SetTransform(const matrix44& Tfm);
	virtual void	GetTransform(vector3& OutPos, quaternion& OutRot) const;
	void			SetTransformChanged(bool Changed);
	bool			IsTransformChanged() const;
};

typedef Ptr<CRigidBody> PRigidBody;

}

#endif
