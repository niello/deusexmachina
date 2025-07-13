#pragma once
#include <Physics/PhysicsObject.h>
#include <LinearMath/btMotionState.h>

// Dynamic object - rigid body simulated by physics. Can be used
// as a transformation source for a scene node.

class btRigidBody; // FIXME: hide behind the facade?

namespace Scene
{
	using PSceneNode = Ptr<class CSceneNode>;
}

namespace Physics
{

class CRigidBody : public CPhysicsObject
{
	RTTI_CLASS_DECL(Physics::CRigidBody, Physics::CPhysicsObject);

protected:

	// To be useful, rigid body must be connected to a scene node to serve as its transformation source
	class CDynamicMotionState : public btMotionState
	{
	protected:

		Scene::PSceneNode _Node;
		rtm::vector4f     _Offset;

	public:

		CDynamicMotionState(const rtm::vector4f& Offset);
		virtual ~CDynamicMotionState() override;

		void SetSceneNode(Scene::PSceneNode&& Node);
		Scene::CSceneNode* GetSceneNode() const { return _Node.Get(); }

		virtual void getWorldTransform(btTransform& worldTrans) const override;
		virtual void setWorldTransform(const btTransform& worldTrans) override;
	};

	CDynamicMotionState _MotionState;

	virtual void AttachToLevelInternal() override;
	virtual void RemoveFromLevelInternal() override;

public:

	CRigidBody(float Mass, CCollisionShape& Shape, CStrID CollisionGroupID = CStrID::Empty, CStrID CollisionMaskID = CStrID::Empty, const rtm::matrix3x4f& InitialTfm = rtm::matrix_identity(), const CPhysicsMaterial& Material = CPhysicsMaterial::Default());
	virtual ~CRigidBody() override;

	void               SetControlledNode(Scene::CSceneNode* pNode);
	Scene::CSceneNode* GetControlledNode() const { return _MotionState.GetSceneNode(); }

	virtual void       SetTransform(const rtm::matrix3x4f& Tfm) override;
	virtual void       GetTransform(rtm::matrix3x4f& OutTfm) const override;
	virtual void       GetGlobalAABB(Math::CAABB& OutBox) const override;
	virtual void       SetActive(bool Active, bool Always = false) override;
	float              GetInvMass() const;
	float              GetMass() const { return 1.f / GetInvMass(); }

	// Access real physical transform, not an interpolated motion state
	rtm::vector4f      GetPhysicalPosition() const;

	// FIXME: hide behind the facade?
	btRigidBody*       GetBtBody() const;
};

typedef Ptr<CRigidBody> PRigidBody;

}
