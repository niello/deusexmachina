#include "MovableCollider.h"
#include <Physics/BulletConv.h>
#include <Physics/PhysicsLevel.h>
#include <Physics/CollisionShape.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{
RTTI_CLASS_IMPL(Physics::CMovableCollider, Core::CObject); //Physics::CPhysicsObject);

// Physics object doesn't know the source of its transform, ot only stores an incoming copy
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
		_Tfm.getOrigin() += VectorToBtVector(Offset);
	}

	virtual void getWorldTransform(btTransform& worldTrans) const { worldTrans = _Tfm; }
	virtual void setWorldTransform(const btTransform& worldTrans) { /* must not be called on static and kinematic objects */ }
};

CMovableCollider::CMovableCollider(CPhysicsLevel& Level, CCollisionShape& Shape, U16 CollisionGroup, U16 CollisionMask, const matrix44& InitialTfm)
	: _Level(&Level)
{
	// Instead of storing strong ref, we manually control refcount and use
	// a pointer from the bullet collision shape
	Shape.AddRef();

	btRigidBody::btRigidBodyConstructionInfo CI(
		0.f,
		new CKinematicMotionState(InitialTfm, Shape.GetOffset()),
		Shape.GetBulletShape());
	//!!!set friction and restitution! for spheres always need rolling friction!

	_pBtObject = new btRigidBody(CI);
	_pBtObject->setCollisionFlags(_pBtObject->getCollisionFlags() | btRigidBody::CF_KINEMATIC_OBJECT);
	_pBtObject->setUserPointer(this);

	_Level->GetBtWorld()->addRigidBody(_pBtObject, CollisionGroup, CollisionMask);
}
//---------------------------------------------------------------------

CMovableCollider::~CMovableCollider()
{
	if (_Level && _pBtObject)
	{
		auto pShape = static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer());

		_Level->GetBtWorld()->removeRigidBody(_pBtObject);
		delete _pBtObject->getMotionState();
		delete _pBtObject;

		// See constructor
		pShape->Release();
	}
}
//---------------------------------------------------------------------

void CMovableCollider::SetActive(bool Active)
{
	_pBtObject->forceActivationState(Active ? DISABLE_DEACTIVATION : WANTS_DEACTIVATION /* DISABLE_SIMULATION */);
}
//---------------------------------------------------------------------

bool CMovableCollider::IsActive() const
{
	return _pBtObject->isActive();
}
//---------------------------------------------------------------------

void CMovableCollider::SetTransform(const matrix44& Tfm)
{
	auto pShape = static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer());
	static_cast<CKinematicMotionState*>(_pBtObject->getMotionState())->SetTransform(Tfm, pShape->GetOffset());
	SetActive(true);
}
//---------------------------------------------------------------------

void CMovableCollider::GetTransform(vector3& OutPos, quaternion& OutRot) const
{
	btTransform Tfm;
	_pBtObject->getMotionState()->getWorldTransform(Tfm);

	auto pShape = static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer());

	OutRot = BtQuatToQuat(Tfm.getRotation());
	OutPos = BtVectorToVector(Tfm.getOrigin()) - pShape->GetOffset();
}
//---------------------------------------------------------------------

// Interpolated AABB from motion state, matches the graphics representation
void CMovableCollider::GetGlobalAABB(CAABB& OutBox) const
{
	btTransform Tfm;
	_pBtObject->getMotionState()->getWorldTransform(Tfm);

	btVector3 Min, Max;
	_pBtObject->getCollisionShape()->getAabb(Tfm, Min, Max);
	OutBox.Min = BtVectorToVector(Min);
	OutBox.Max = BtVectorToVector(Max);
}
//---------------------------------------------------------------------

}