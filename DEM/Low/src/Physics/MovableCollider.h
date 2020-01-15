#pragma once
#include <Physics/PhysicsObject.h>

// Moving collision object inherits transform from the scene node, collides with
// dynamic bodies as a moving object, but doesn't respond to collisions.
// Use this type of objects to represent objects controlled by an animation,
// by user input or any other non-physics controller.

namespace Physics
{

class CMovableCollider : public CPhysicsObject
{
	RTTI_CLASS_DECL;

public:

	CMovableCollider(CPhysicsLevel& Level, CCollisionShape& Shape, U16 CollisionGroup, U16 CollisionMask, const matrix44& InitialTfm = matrix44::Identity);
	virtual ~CMovableCollider() override;

	void SetActive(bool Active);

	// FIXME: need consistency! both matrix or both vector+quat!
	void GetTransform(vector3& OutPos, quaternion& OutRot) const;
	void SetTransform(const matrix44& Tfm);
	void GetGlobalAABB(CAABB& OutBox) const;
};

typedef Ptr<CMovableCollider> PMovableCollider;

}
