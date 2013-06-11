#include "CollisionObjMoving.h"

#include <Physics/BulletConv.h>
#include <Physics/PhysicsWorld.h>
#include <Physics/MotionStateKinematic.h>
#include <mathlib/bbox.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CCollisionObjMoving, Physics::CPhysicsObj);

bool CCollisionObjMoving::Init(const Data::CParams& Desc, const vector3& Offset)
{
	if (!CPhysicsObj::Init(Desc, Offset)) FAIL;
	return InternalInit();
}
//---------------------------------------------------------------------

bool CCollisionObjMoving::Init(CCollisionShape& CollShape, ushort CollGroup, ushort CollMask, const vector3& Offset)
{
	if (!CPhysicsObj::Init(CollShape, CollGroup, CollMask, Offset)) FAIL;
	return InternalInit();
}
//---------------------------------------------------------------------

void CCollisionObjMoving::Term()
{
	InternalTerm();
	CPhysicsObj::Term();
}
//---------------------------------------------------------------------

bool CCollisionObjMoving::InternalInit()
{
	CMotionStateKinematic* pMS = new CMotionStateKinematic;
	btRigidBody::btRigidBodyConstructionInfo CI(0.f, pMS, Shape->GetBtShape());

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
		((btRigidBody*)pBtCollObj)->setMotionState(NULL);
		delete pMS;
	}
}
//---------------------------------------------------------------------

bool CCollisionObjMoving::AttachToLevel(CPhysicsWorld& World)
{
	if (!CPhysicsObj::AttachToLevel(World)) FAIL;

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
	CPhysicsObj::RemoveFromLevel();
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
		pMotionState->Tfm.getOrigin().m_floats[0] += ShapeOffset.x;
		pMotionState->Tfm.getOrigin().m_floats[1] += ShapeOffset.y;
		pMotionState->Tfm.getOrigin().m_floats[2] += ShapeOffset.z;
	}
	else CPhysicsObj::SetTransform(Tfm);
}
//---------------------------------------------------------------------

void CCollisionObjMoving::GetTransform(btTransform& Out) const
{
	n_assert_dbg(pBtCollObj);

	CMotionStateKinematic* pMotionState = (CMotionStateKinematic*)((btRigidBody*)pBtCollObj)->getMotionState();
	if (pMotionState)
	{
		Out = pMotionState->Tfm;
		Out.getOrigin().m_floats[0] -= ShapeOffset.x;
		Out.getOrigin().m_floats[1] -= ShapeOffset.y;
		Out.getOrigin().m_floats[2] -= ShapeOffset.z;
	}
	else CPhysicsObj::GetTransform(Out);
}
//---------------------------------------------------------------------

}