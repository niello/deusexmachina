#pragma once
#include <Physics/PhysicsObject.h>

// Static object - collider that can't move. Transform changes are discrete and manual.
// It is the most effective representation of static environment.

namespace Physics
{

class CStaticCollider : public CPhysicsObject
{
	RTTI_CLASS_DECL(Physics::CStaticCollider, Physics::CPhysicsObject);

protected:

	virtual void AttachToLevelInternal() override;
	virtual void RemoveFromLevelInternal() override;

public:

	CStaticCollider(CCollisionShape& Shape, CStrID CollisionGroupID = CStrID::Empty, CStrID CollisionMaskID = CStrID::Empty, const rtm::matrix3x4f& InitialTfm = rtm::matrix_identity(), const CPhysicsMaterial& Material = CPhysicsMaterial::Default());
	virtual ~CStaticCollider() override;

	virtual void SetTransform(const rtm::matrix3x4f& Tfm) override;
	virtual void GetTransform(rtm::matrix3x4f& OutTfm) const override;
	virtual void GetGlobalAABB(Math::CAABB& OutBox) const override;
	virtual void SetActive(bool Active, bool Always = false) override { /* Static object is never active */ }
};

typedef Ptr<CStaticCollider> PStaticCollider;

}
