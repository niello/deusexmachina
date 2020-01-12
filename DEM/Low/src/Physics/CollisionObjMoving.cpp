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
RTTI_CLASS_IMPL(Physics::CCollisionObjMoving, Physics::CPhysicsObject);

bool CCollisionObjMoving::Init(const Data::CParams& Desc, const vector3& Offset)
{
	if (!CPhysicsObject::Init(Desc, Offset)) FAIL;
	return InternalInit();
}
//---------------------------------------------------------------------

bool CCollisionObjMoving::Init(CCollisionShape& CollShape, U16 CollGroup, U16 CollMask, const vector3& Offset)
{
	if (!CPhysicsObject::Init(CollShape, CollGroup, CollMask, Offset)) FAIL;
	return InternalInit();
}
//---------------------------------------------------------------------

void CCollisionObjMoving::Term()
{
	InternalTerm();
	CPhysicsObject::Term();
}
//---------------------------------------------------------------------

bool CCollisionObjMoving::InternalInit()
{
	btRigidBody::btRigidBodyConstructionInfo CI(
		0.f,
		new CMotionStateKinematic(),
		Shape->GetBtShape());
	//!!!set friction and restitution!

	pBtCollObj = new btRigidBody(CI);
	pBtCollObj->setCollisionFlags(pBtCollObj->getCollisionFlags() | btRigidBody::CF_KINEMATIC_OBJECT);
	pBtCollObj->setUserPointer(this);

	//???or activate only when tfm changes?
	pBtCollObj->setActivationState(DISABLE_DEACTIVATION);

	OK;
}
//---------------------------------------------------------------------

void CCollisionObjMoving::InternalTerm()
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

bool CCollisionObjMoving::AttachToLevel(CPhysicsLevel& World)
{
	if (!CPhysicsObject::AttachToLevel(World)) FAIL;

	// Enforce offline transform update to be taken into account
	btRigidBody* pRB = (btRigidBody*)pBtCollObj;
	pRB->setMotionState(pRB->getMotionState());
	pWorld->GetBtWorld()->addRigidBody(pRB, Group, Mask);

	OK;
}
//---------------------------------------------------------------------

void CCollisionObjMoving::RemoveFromLevel()
{
	if (!pWorld || !pWorld->GetBtWorld()) return;
	pWorld->GetBtWorld()->removeRigidBody((btRigidBody*)pBtCollObj);
	CPhysicsObject::RemoveFromLevel();
}
//---------------------------------------------------------------------

void CCollisionObjMoving::SetTransform(const matrix44& Tfm)
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

void CCollisionObjMoving::GetTransform(btTransform& Out) const
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