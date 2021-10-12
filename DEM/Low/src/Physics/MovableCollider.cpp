#include "MovableCollider.h"
#include <Physics/BulletConv.h>
#include <Physics/PhysicsLevel.h>
#include <Physics/CollisionShape.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{

//???FIXME: need this? need initial tfm? high chances that identity is passed because world tfm is not yet calculated!
void CMovableCollider::CKinematicMotionState::SetTransform(const matrix44& NewTfm, const vector3& Offset)
{
	_Tfm = TfmToBtTfm(NewTfm);
	_Tfm.getOrigin() = _Tfm * VectorToBtVector(Offset);
}
//---------------------------------------------------------------------

CMovableCollider::CMovableCollider(CCollisionShape& Shape, CStrID CollisionGroupID, CStrID CollisionMaskID, const matrix44& InitialTfm, const CPhysicsMaterial& Material)
	: CPhysicsObject(CollisionGroupID, CollisionMaskID)
	, _MotionState(InitialTfm, Shape.GetOffset())
{
	// NB: mass must be zero for kinematic objects
	btRigidBody::btRigidBodyConstructionInfo CI(0.f, &_MotionState, Shape.GetBulletShape());
	ConstructInternal(new btRigidBody(CI), Material);
	_pBtObject->setCollisionFlags(_pBtObject->getCollisionFlags() | btRigidBody::CF_KINEMATIC_OBJECT);
}
//---------------------------------------------------------------------

CMovableCollider::~CMovableCollider()
{
	if (_Level) _Level->GetBtWorld()->removeRigidBody(static_cast<btRigidBody*>(_pBtObject));
}
//---------------------------------------------------------------------

void CMovableCollider::AttachToLevelInternal()
{
	const U16 Group = _Level->CollisionGroups.GetMask(_CollisionGroupID ? _CollisionGroupID.CStr() : "PhysicalDynamic");
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
	if (PrepareTransform(Tfm, _MotionState._Tfm))
		_pBtObject->forceActivationState(DISABLE_DEACTIVATION);
}
//---------------------------------------------------------------------

void CMovableCollider::GetTransform(matrix44& OutTfm) const
{
	btTransform Tfm;
	_MotionState.getWorldTransform(Tfm);

	auto pShape = static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer());

	OutTfm = BtTfmToTfm(Tfm);
	OutTfm.Translation() = OutTfm * (-pShape->GetOffset());
}
//---------------------------------------------------------------------

// Interpolated AABB from motion state, matches the graphics representation
void CMovableCollider::GetGlobalAABB(CAABB& OutBox) const
{
	btTransform Tfm;
	_MotionState.getWorldTransform(Tfm);

	btVector3 Min, Max;
	_pBtObject->getCollisionShape()->getAabb(Tfm, Min, Max);
	OutBox.Min = BtVectorToVector(Min);
	OutBox.Max = BtVectorToVector(Max);
}
//---------------------------------------------------------------------

}
