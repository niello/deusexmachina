#pragma once
#include <Physics/PhysicsObject.h>

// Kinematic object - moving collider that inherits transform from the scene node,
// collides with dynamic bodies as a moving object, but doesn't respond to collisions.
// Use this type of objects to represent objects controlled by an animation,
// by user input or any other non-physics controller.

namespace Physics
{

class CMovableCollider : public CPhysicsObject
{
	RTTI_CLASS_DECL(Physics::CMovableCollider, Physics::CPhysicsObject);

protected:

	virtual void AttachToLevelInternal() override;
	virtual void RemoveFromLevelInternal() override;

public:

	CMovableCollider(CCollisionShape& Shape, CStrID CollisionGroupID = CStrID::Empty, CStrID CollisionMaskID = CStrID::Empty, const matrix44& InitialTfm = matrix44::Identity, const CPhysicsMaterial& Material = CPhysicsMaterial::Default());
	virtual ~CMovableCollider() override;

	virtual void SetTransform(const matrix44& Tfm) override;
	virtual void GetTransform(matrix44& OutTfm) const override;
	virtual void GetGlobalAABB(CAABB& OutBox) const override;
	virtual void SetActive(bool Active, bool Always = false) override;
};

typedef Ptr<CMovableCollider> PMovableCollider;

}
