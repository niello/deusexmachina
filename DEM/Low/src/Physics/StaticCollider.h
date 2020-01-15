#pragma once
#include <Physics/PhysicsObject.h>

// Static object - collider that can't move. Transform changes are discrete and manual.
// It is the most effective representation of static environment.

namespace Physics
{

class CStaticCollider : public CPhysicsObject
{
	RTTI_CLASS_DECL;

public:

	CStaticCollider(CPhysicsLevel& Level, CCollisionShape& Shape, U16 CollisionGroup, U16 CollisionMask, const matrix44& InitialTfm = matrix44::Identity);
	virtual ~CStaticCollider() override;

	virtual void SetActive(bool Active, bool Always = false) { /* Static object is never active */ }
};

typedef Ptr<CStaticCollider> PStaticCollider;

}
