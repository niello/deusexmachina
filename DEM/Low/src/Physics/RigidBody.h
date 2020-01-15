#pragma once
#include <Physics/PhysicsObject.h>

// Rigid body simulated by physics. Can be used as a transformation source for a scene node.

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

	void SetControlledNode(Scene::CSceneNode* pNode);

	virtual void	SetTransform(const matrix44& Tfm);
	virtual void	GetTransform(vector3& OutPos, quaternion& OutRot) const;
	float			GetInvMass() const;
	float			GetMass() const { return 1.f / GetInvMass(); }
	void            SetActive(bool Active, bool Always);
};

typedef Ptr<CRigidBody> PRigidBody;

}
