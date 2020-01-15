#pragma once
#include <Physics/PhysicsObject.h>

// Static collision object can't move. Transform changes are discrete and manual.
// It is the most effective representation of static environment.

namespace Physics
{

class CStaticCollider : public CPhysicsObject
{
	RTTI_CLASS_DECL;

public:

	CStaticCollider(CPhysicsLevel& Level, CCollisionShape& Shape, U16 CollisionGroup, U16 CollisionMask, const matrix44& InitialTfm = matrix44::Identity);
	virtual ~CStaticCollider() override;
};

typedef Ptr<CStaticCollider> PStaticCollider;

}
