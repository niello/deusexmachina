#pragma once
#include <Core/Object.h>
#include <Math/AABB.h>

// Static collision object can't move. Transform changes are discrete and manual.
// It is the most effective representation of static environment.

class btCollisionObject;

namespace Physics
{
typedef Ptr<class CPhysicsLevel> PPhysicsLevel;
class CCollisionShape;

class CStaticCollider : public Core::CObject
{
	RTTI_CLASS_DECL;

protected:

	btCollisionObject* _pBtObject = nullptr;
	PPhysicsLevel      _Level;

public:

	CStaticCollider(CPhysicsLevel& Level, CCollisionShape& Shape, U16 CollisionGroup, U16 CollisionMask, const matrix44& InitialTfm = matrix44::Identity);
	virtual ~CStaticCollider() override;
};

typedef Ptr<CStaticCollider> PStaticCollider;

}
