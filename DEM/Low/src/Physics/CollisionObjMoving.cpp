#include "CollisionObjMoving.h"

#include <Physics/BulletConv.h>
#include <Physics/PhysicsLevel.h>
#include <Physics/MotionStateKinematic.h>
#include <Math/AABB.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{
RTTI_CLASS_IMPL(Physics::CMovableCollider, Physics::CPhysicsObject);

bool CMovableCollider::Init(const Data::CParams& Desc, const vector3& Offset)
{
	if (!CPhysicsObject::Init(Desc, Offset)) FAIL;
	return InternalInit();
}
//---------------------------------------------------------------------

bool CMovableCollider::Init(CCollisionShape& CollShape, U16 CollGroup, U16 CollMask, const vector3& Offset)
{
	if (!CPhysicsObject::Init(CollShape, CollGroup, CollMask, Offset)) FAIL;
	return InternalInit();
}
//---------------------------------------------------------------------

void CMovableCollider::Term()
{
	InternalTerm();
	CPhysicsObject::Term();
}
//---------------------------------------------------------------------

bool CMovableCollider::InternalInit()
{
	btRigidBody::btRigidBodyConstructionInfo CI(
		0.f,
		new CMotionStateKinematic(),
		Shape->GetBulletShape());
	//!!!set friction and restitution!

	pBtCollObj = new btRigidBody(CI);
	pBtCollObj->setCollisionFlags(pBtCollObj->getCollisionFlags() | btRigidBody::CF_KINEMATIC_OBJECT);
	pBtCollObj->setUserPointer(this);

	//???or activate only when tfm changes?
	pBtCollObj->setActivationState(DISABLE_DEACTIVATION);

	OK;
}
//---------------------------------------------------------------------

void CMovableCollider::InternalTerm()
{
	if (!pBtCollObj) return;

	btMotionState* pMS = ((btRigidBody*)pBtCollObj)->getMotionState();
	if (pMS)
	{
		((btRigidBody*)pBtCollObj)->setMotionState(nullptr);
		delete pMS;
	}
}
//---------------------------------------------------------------------

bool CMovableCollider::AttachToLevel(CPhysicsLevel& World)
{
	if (!CPhysicsObject::AttachToLevel(World)) FAIL;

	// Enforce offline transform update to be taken into account
	btRigidBody* pRB = (btRigidBody*)pBtCollObj;
	pRB->setMotionState(pRB->getMotionState());
	pWorld->GetBtWorld()->addRigidBody(pRB, Group, Mask);

	OK;
}
//---------------------------------------------------------------------

void CMovableCollider::RemoveFromLevel()
{
	if (!pWorld || !pWorld->GetBtWorld()) return;
	pWorld->GetBtWorld()->removeRigidBody((btRigidBody*)pBtCollObj);
	CPhysicsObject::RemoveFromLevel();
}
//---------------------------------------------------------------------

void CMovableCollider::SetTransform(const matrix44& Tfm)
{
	n_assert_dbg(pBtCollObj);

	//???activate physics body only when tfm changed?

	CMotionStateKinematic* pMotionState = (CMotionStateKinematic*)((btRigidBody*)pBtCollObj)->getMotionState();
	if (pMotionState)
	{
		pMotionState->Tfm = TfmToBtTfm(Tfm);
		pMotionState->Tfm.getOrigin() = pMotionState->Tfm * VectorToBtVector(ShapeOffset);
	}
	else CPhysicsObject::SetTransform(Tfm);
}
//---------------------------------------------------------------------

void CMovableCollider::GetTransform(btTransform& Out) const
{
	n_assert_dbg(pBtCollObj);

	CMotionStateKinematic* pMotionState = (CMotionStateKinematic*)((btRigidBody*)pBtCollObj)->getMotionState();
	if (pMotionState)
	{
		Out = pMotionState->Tfm;
		Out.getOrigin() = Out * VectorToBtVector(-ShapeOffset);
	}
	else CPhysicsObject::GetTransform(Out);
}
//---------------------------------------------------------------------

}