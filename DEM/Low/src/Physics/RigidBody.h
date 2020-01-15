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

public:

	CRigidBody(CPhysicsLevel& Level, CCollisionShape& Shape, U16 CollisionGroup, U16 CollisionMask, float Mass, const matrix44& InitialTfm = matrix44::Identity);
	virtual ~CRigidBody() override;

	void         SetControlledNode(Scene::CSceneNode* pNode);

	virtual void SetTransform(const matrix44& Tfm);
	virtual void GetTransform(vector3& OutPos, quaternion& OutRot) const;
	virtual void SetActive(bool Active, bool Always = false) override;
	float        GetInvMass() const;
	float        GetMass() const { return 1.f / GetInvMass(); }
};

typedef Ptr<CRigidBody> PRigidBody;

}
