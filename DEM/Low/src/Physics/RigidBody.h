#pragma once
#include <Core/Object.h>
#include <Math/AABB.h>
//#include <Physics/PhysicsObject.h>

// Rigid body simulated by physics. Can be used as a transformation source for a scene node.

class btRigidBody;

namespace Physics
{
typedef Ptr<class CPhysicsLevel> PPhysicsLevel;
class CCollisionShape;

class CRigidBody: public Core::CObject //CPhysicsObject
{
	RTTI_CLASS_DECL;

protected:

	btRigidBody*    _pBtObject = nullptr;
	PPhysicsLevel   _Level;

public:

	CRigidBody(CPhysicsLevel& Level, CCollisionShape& Shape, U16 CollisionGroup, U16 CollisionMask, float Mass, const matrix44& InitialTfm = matrix44::Identity);
	virtual ~CRigidBody() override;

	virtual void	SetTransform(const matrix44& Tfm);
	virtual void	GetTransform(vector3& OutPos, quaternion& OutRot) const;
	void			SetTransformChanged(bool Changed);
	bool			IsTransformChanged() const;
	float			GetInvMass() const;
	float			GetMass();
	bool			IsActive() const;
	bool			IsAlwaysActive() const;
	bool			IsAlwaysInactive() const;
	void			MakeActive();
	void			MakeAlwaysActive();
	void			MakeAlwaysInactive();
};

typedef Ptr<CRigidBody> PRigidBody;

}
