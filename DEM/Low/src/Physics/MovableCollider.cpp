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
void CMovableCollider::CKinematicMotionState::SetTransform(const rtm::matrix3x4f& NewTfm, const rtm::vector4f& Offset)
{
	_Tfm = Math::ToBullet(NewTfm);
	_Tfm.getOrigin() = _Tfm * Math::ToBullet3(Offset);
}
//---------------------------------------------------------------------

CMovableCollider::CMovableCollider(CCollisionShape& Shape, CStrID CollisionGroupID, CStrID CollisionMaskID, const rtm::matrix3x4f& InitialTfm, const CPhysicsMaterial& Material)
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
	const auto Group = _Level->CollisionGroups.GetMask(_CollisionGroupID ? _CollisionGroupID.CStr() : "PhysicalDynamic");
	const auto Mask = _Level->CollisionGroups.GetMask(_CollisionMaskID ? _CollisionMaskID.CStr() : "All");
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

void CMovableCollider::SetTransform(const rtm::matrix3x4f& Tfm)
{
	if (PrepareTransform(Tfm, _MotionState._Tfm))
		_pBtObject->forceActivationState(DISABLE_DEACTIVATION);
}
//---------------------------------------------------------------------

void CMovableCollider::GetTransform(rtm::matrix3x4f& OutTfm) const
{
	btTransform Tfm;
	_MotionState.getWorldTransform(Tfm);

	auto pShape = static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer());

	OutTfm = Math::FromBullet(Tfm);
	OutTfm.w_axis = rtm::matrix_mul_point3(rtm::vector_neg(pShape->GetOffset()), OutTfm);
}
//---------------------------------------------------------------------

// Interpolated AABB from motion state, matches the graphics representation
void CMovableCollider::GetGlobalAABB(Math::CAABB& OutBox) const
{
	btTransform Tfm;
	_MotionState.getWorldTransform(Tfm);

	btVector3 Min, Max;
	_pBtObject->getCollisionShape()->getAabb(Tfm, Min, Max);
	OutBox = Math::AABBFromMinMax(Math::FromBullet(Min), Math::FromBullet(Max));
}
//---------------------------------------------------------------------

}
