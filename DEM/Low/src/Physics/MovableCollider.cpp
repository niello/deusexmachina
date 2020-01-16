#include "MovableCollider.h"
#include <Physics/BulletConv.h>
#include <Physics/PhysicsLevel.h>
#include <Physics/CollisionShape.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{
RTTI_CLASS_IMPL(Physics::CMovableCollider, Physics::CPhysicsObject);

// Physics object doesn't know the source of its transform, it only stores an incoming copy
class CKinematicMotionState: public btMotionState
{
protected:

	btTransform _Tfm;

public:

	BT_DECLARE_ALIGNED_ALLOCATOR();

	CKinematicMotionState(const matrix44& InitialTfm, const vector3& Offset) { SetTransform(InitialTfm, Offset); }

	void SetTransform(const matrix44& NewTfm, const vector3& Offset)
	{
		_Tfm = TfmToBtTfm(NewTfm);
		_Tfm.getOrigin() = _Tfm * VectorToBtVector(Offset);
	}

	virtual void getWorldTransform(btTransform& worldTrans) const override { worldTrans = _Tfm; }
	virtual void setWorldTransform(const btTransform& worldTrans) override { /* must not be called on static and kinematic objects */ }
};

CMovableCollider::CMovableCollider(CCollisionShape& Shape, CStrID CollisionGroupID, CStrID CollisionMaskID, const matrix44& InitialTfm, const CPhysicsMaterial& Material)
	: CPhysicsObject(CollisionGroupID, CollisionMaskID)
{
	// Instead of storing strong ref, we manually control refcount and use
	// a pointer from the bullet collision shape
	Shape.AddRef();

	btRigidBody::btRigidBodyConstructionInfo CI(
		0.f,
		new CKinematicMotionState(InitialTfm, Shape.GetOffset()),
		Shape.GetBulletShape());

	SetupInternalObject(new btRigidBody(CI), Shape, Material);
	_pBtObject->setCollisionFlags(_pBtObject->getCollisionFlags() | btRigidBody::CF_KINEMATIC_OBJECT);
}
//---------------------------------------------------------------------

CMovableCollider::~CMovableCollider()
{
	if (_Level) _Level->GetBtWorld()->removeRigidBody(static_cast<btRigidBody*>(_pBtObject));

	auto pShape = static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer());
	delete static_cast<btRigidBody*>(_pBtObject)->getMotionState();
	delete _pBtObject;
	pShape->Release(); // See constructor
}
//---------------------------------------------------------------------

void CMovableCollider::AttachToLevelInternal()
{
	const U16 Group = _Level->CollisionGroups.GetMask(_CollisionGroupID ? _CollisionGroupID.CStr() : "Default");
	const U16 Mask = _Level->CollisionGroups.GetMask(_CollisionMaskID ? _CollisionMaskID.CStr() : "All");
	_Level->GetBtWorld()->addRigidBody(static_cast<btRigidBody*>(_pBtObject), Group, Mask);
}
//---------------------------------------------------------------------

void CMovableCollider::RemoveFromLevelInternal()
{
	_Level->GetBtWorld()->removeRigidBody(static_cast<btRigidBody*>(_pBtObject));
}
//---------------------------------------------------------------------

void CMovableCollider::SetActive(bool Active, bool Always)
{
	// Kinematic object must never be auto-deactivated
	if (Active)
		_pBtObject->forceActivationState(DISABLE_DEACTIVATION);
	else
		_pBtObject->forceActivationState(Always ? DISABLE_SIMULATION : WANTS_DEACTIVATION);
}
//---------------------------------------------------------------------

void CMovableCollider::SetTransform(const matrix44& Tfm)
{
	_pBtObject->forceActivationState(DISABLE_DEACTIVATION);
	auto pShape = static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer());
	static_cast<CKinematicMotionState*>(static_cast<btRigidBody*>(_pBtObject)->getMotionState())->SetTransform(Tfm, pShape->GetOffset());
}
//---------------------------------------------------------------------

void CMovableCollider::GetTransform(matrix44& OutTfm) const
{
	btTransform Tfm;
	static_cast<btRigidBody*>(_pBtObject)->getMotionState()->getWorldTransform(Tfm);

	auto pShape = static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer());

	OutTfm = BtTfmToTfm(Tfm);
	OutTfm.Translation() = OutTfm * (-pShape->GetOffset());
}
//---------------------------------------------------------------------

// Interpolated AABB from motion state, matches the graphics representation
void CMovableCollider::GetGlobalAABB(CAABB& OutBox) const
{
	btTransform Tfm;
	static_cast<btRigidBody*>(_pBtObject)->getMotionState()->getWorldTransform(Tfm);

	btVector3 Min, Max;
	_pBtObject->getCollisionShape()->getAabb(Tfm, Min, Max);
	OutBox.Min = BtVectorToVector(Min);
	OutBox.Max = BtVectorToVector(Max);
}
//---------------------------------------------------------------------

}