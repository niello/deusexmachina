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
		vector3           _Offset;

	public:

		CDynamicMotionState(const vector3& Offset);
		virtual ~CDynamicMotionState() override;

		void SetSceneNode(Scene::PSceneNode&& Node) { _Node = std::move(Node); }
		Scene::CSceneNode* GetSceneNode() const { return _Node.Get(); }

		virtual void getWorldTransform(btTransform& worldTrans) const override;
		virtual void setWorldTransform(const btTransform& worldTrans) override;
	};

	CDynamicMotionState _MotionState;

	virtual void AttachToLevelInternal() override;
	virtual void RemoveFromLevelInternal() override;

public:

	CRigidBody(float Mass, CCollisionShape& Shape, CStrID CollisionGroupID = CStrID::Empty, CStrID CollisionMaskID = CStrID::Empty, const matrix44& InitialTfm = matrix44::Identity, const CPhysicsMaterial& Material = CPhysicsMaterial::Default());
	virtual ~CRigidBody() override;

	void               SetControlledNode(Scene::CSceneNode* pNode);
	Scene::CSceneNode* GetControlledNode() const { return _MotionState.GetSceneNode(); }

	virtual void       SetTransform(const matrix44& Tfm) override;
	virtual void       GetTransform(matrix44& OutTfm) const override;
	virtual void       GetGlobalAABB(CAABB& OutBox) const override;
	virtual void       SetActive(bool Active, bool Always = false) override;
	float              GetInvMass() const;
	float              GetMass() const { return 1.f / GetInvMass(); }

	// FIXME: hide behind the facade?
	btRigidBody*       GetBtBody() const;
};

typedef Ptr<CRigidBody> PRigidBody;

}
