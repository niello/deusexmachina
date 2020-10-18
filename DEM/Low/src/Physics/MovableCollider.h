#pragma once
#include <Physics/PhysicsObject.h>
#include <LinearMath/btMotionState.h>

// Kinematic object - moving collider that inherits transform from the scene node,
// collides with dynamic bodies as a moving object, but doesn't respond to collisions.
// Use this type of objects to represent objects controlled by an animation,
// by user input or any other non-physics controller.

namespace Physics
{

ATTRIBUTE_ALIGNED16(class)
CMovableCollider : public CPhysicsObject
{
	RTTI_CLASS_DECL(Physics::CMovableCollider, Physics::CPhysicsObject);

protected:

	// Physics object doesn't know the source of its transform, it only stores an incoming copy
	ATTRIBUTE_ALIGNED16(class)
	CKinematicMotionState : public btMotionState
	{
	public:

		btTransform _Tfm;

		BT_DECLARE_ALIGNED_ALLOCATOR();

		CKinematicMotionState(const matrix44& InitialTfm, const vector3& Offset) { SetTransform(InitialTfm, Offset); }

		//???FIXME: need this? need initial tfm? high chances that identity is passed because world tfm is not yet calculated!
		void SetTransform(const matrix44& NewTfm, const vector3& Offset);

		virtual void getWorldTransform(btTransform& worldTrans) const override { worldTrans = _Tfm; }
		virtual void setWorldTransform(const btTransform& worldTrans) override { /* must not be called on static and kinematic objects */ }
	};

	CKinematicMotionState _MotionState;

	virtual void AttachToLevelInternal() override;
	virtual void RemoveFromLevelInternal() override;

public:

	BT_DECLARE_ALIGNED_ALLOCATOR();

	CMovableCollider(CCollisionShape& Shape, CStrID CollisionGroupID = CStrID::Empty, CStrID CollisionMaskID = CStrID::Empty, const matrix44& InitialTfm = matrix44::Identity, const CPhysicsMaterial& Material = CPhysicsMaterial::Default());
	virtual ~CMovableCollider() override;

	virtual void SetTransform(const matrix44& Tfm) override;
	virtual void GetTransform(matrix44& OutTfm) const override;
	virtual void GetGlobalAABB(CAABB& OutBox) const override;
	virtual void SetActive(bool Active, bool Always = false) override;
};

typedef Ptr<CMovableCollider> PMovableCollider;

}
