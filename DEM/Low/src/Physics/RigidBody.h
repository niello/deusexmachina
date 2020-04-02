#pragma once
#include <Physics/PhysicsObject.h>

// Dynamic object - rigid body simulated by physics. Can be used
// as a transformation source for a scene node.

namespace Scene
{
	class CSceneNode;
}

namespace Physics
{

class CRigidBody : public CPhysicsObject
{
	RTTI_CLASS_DECL;

protected:

	virtual void AttachToLevelInternal() override;
	virtual void RemoveFromLevelInternal() override;

public:

	CRigidBody(float Mass, CCollisionShape& Shape, CStrID CollisionGroupID = CStrID::Empty, CStrID CollisionMaskID = CStrID::Empty, const matrix44& InitialTfm = matrix44::Identity, const CPhysicsMaterial& Material = CPhysicsMaterial::Default());
	virtual ~CRigidBody() override;

	void               SetControlledNode(Scene::CSceneNode* pNode);
	Scene::CSceneNode* GetControlledNode() const;

	virtual void       SetTransform(const matrix44& Tfm) override;
	virtual void       GetTransform(matrix44& OutTfm) const override;
	virtual void       GetGlobalAABB(CAABB& OutBox) const override;
	virtual void       SetActive(bool Active, bool Always = false) override;
	float              GetInvMass() const;
	float              GetMass() const { return 1.f / GetInvMass(); }
};

typedef Ptr<CRigidBody> PRigidBody;

}
