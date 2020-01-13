#pragma once
#include <Core/Object.h>
#include <Math/AABB.h>

// Moving collision object inherits transform from the scene node, collides with
// dynamic bodies as a moving object, but doesn't respond to collisions.
// Use this type of objects to represent objects controlled by an animation,
// by user input or any other non-physics controller.

class btRigidBody;

namespace Physics
{
typedef Ptr<class CPhysicsLevel> PPhysicsLevel;
typedef Ptr<class CCollisionShape> PCollisionShape;

class CMovableCollider : public Core::CObject
{
	RTTI_CLASS_DECL;

protected:

	btRigidBody*    _pBtObject = nullptr;
	PPhysicsLevel   _Level;
	PCollisionShape _Shape;

public:

	CMovableCollider(CPhysicsLevel& Level, CCollisionShape& Shape, U16 CollisionGroup, U16 CollisionMask, const matrix44& InitialTfm = matrix44::Identity);
	virtual ~CMovableCollider() override;

	void SetActive(bool Active);
	bool IsActive() const;

	// FIXME: need consistency! both matrix or both vector+quat!
	void GetTransform(vector3& OutPos, quaternion& OutRot) const;
	void SetTransform(const matrix44& Tfm);
	void GetGlobalAABB(CAABB& OutBox) const;
};

typedef Ptr<CMovableCollider> PMovableCollider;

}
