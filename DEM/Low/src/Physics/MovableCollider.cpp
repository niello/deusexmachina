#include "MovableCollider.h"
#include <Physics/BulletConv.h>
#include <Physics/PhysicsLevel.h>
#include <Physics/CollisionShape.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{

// Physics object doesn't know the source of its transform, it only stores an incoming copy
ATTRIBUTE_ALIGNED16(class)
CKinematicMotionState: public btMotionState
{
public:

	btTransform _Tfm;

	BT_DECLARE_ALIGNED_ALLOCATOR();

	CKinematicMotionState(const matrix44& InitialTfm, const vector3& Offset) { SetTransform(InitialTfm, Offset); }

	//???FIXME: need this? need initial tfm? high chances that identity is passed because world tfm is not yet calculated!
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
	vector3& AxisX = Tfm.AxisX();
	vector3& AxisY = Tfm.AxisY();
	vector3& AxisZ = Tfm.AxisZ();

	const float ScalingX = AxisX.Length();
	const float ScalingY = AxisY.Length();
	const float ScalingZ = AxisZ.Length();

	if (!UnshareShapeIfNecessary({ ScalingX, ScalingY, ScalingZ })) return;

	_pBtObject->forceActivationState(DISABLE_DEACTIVATION);

	auto pShape = static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer());

	//???TODO PERF: mul reciprocals instead of divisions? is it still actual to do 1 div and 3 mul instead of 3 div?
	//???TODO PERF: optimize origin? VectorToBtVector(Offset).dot3(m_basis[0], m_basis[1], m_basis[2]) + VectorToBtVector(Tfm.Translation())
	auto& OutTfm = static_cast<CKinematicMotionState*>(static_cast<btRigidBody*>(_pBtObject)->getMotionState())->_Tfm;
	OutTfm.setBasis(
		btMatrix3x3(
			AxisX.x / ScalingX, AxisY.x / ScalingY, AxisZ.x / ScalingZ,
			AxisX.y / ScalingX, AxisY.y / ScalingY, AxisZ.y / ScalingZ,
			AxisX.z / ScalingX, AxisY.z / ScalingY, AxisZ.z / ScalingZ));
	OutTfm.setOrigin(VectorToBtVector(Tfm.Translation()));
	OutTfm.getOrigin() = OutTfm * VectorToBtVector(pShape->GetOffset());
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
