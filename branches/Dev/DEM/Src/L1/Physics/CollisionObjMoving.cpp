#include "CollisionObjMoving.h"

#include <Physics/BulletConv.h>
#include <Physics/PhysicsWorld.h>
#include <Physics/MotionStateFromNode.h>
#include <mathlib/bbox.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CCollisionObjMoving, Physics::CCollisionObj);

bool CCollisionObjMoving::Init(const Data::CParams& Desc, const vector3& Offset)
{
	if (!CCollisionObj::Init(Desc, Offset)) FAIL;

	//!!!create motion state!
	//???does the motion state gettfm when object is attached to level?

	btRigidBody::btRigidBodyConstructionInfo CI(0.f, NULL, Shape->GetBtShape());

	//!!!set friction and restitution!

	pBtCollObj = new btRigidBody(CI);
	pBtCollObj->setCollisionFlags(pBtCollObj->getCollisionFlags() | btRigidBody::CF_KINEMATIC_OBJECT);
	pBtCollObj->setUserPointer(this);

	//???or activate only when tfm changes?
	pBtCollObj->setActivationState(DISABLE_DEACTIVATION);

	OK;
}
//---------------------------------------------------------------------

void CCollisionObjMoving::Term()
{
	btMotionState* pMS = pBtCollObj ? ((btRigidBody*)pBtCollObj)->getMotionState() : NULL;

	CCollisionObj::Term();

	if (pMS) delete pMS;
}
//---------------------------------------------------------------------

bool CCollisionObjMoving::AttachToLevel(CPhysicsWorld& World)
{
	if (!CCollisionObj::AttachToLevel(World)) FAIL;
	pWorld->GetBtWorld()->addRigidBody((btRigidBody*)pBtCollObj, Group, Mask);
	OK;
}
//---------------------------------------------------------------------

void CCollisionObjMoving::RemoveFromLevel()
{
	if (!pWorld || !pWorld->GetBtWorld()) return;
	pWorld->GetBtWorld()->removeRigidBody((btRigidBody*)pBtCollObj);
	CCollisionObj::RemoveFromLevel();
}
//---------------------------------------------------------------------

void CCollisionObjMoving::SetTransform(const matrix44& Tfm)
{
	n_assert_dbg(pBtCollObj);

	btMotionState* pMotionState = ((btRigidBody*)pBtCollObj)->getMotionState();
	if (pMotionState)
	{
		// set to motion state cache
	}
	else CCollisionObj::SetTransform(Tfm);
}
//---------------------------------------------------------------------

void CCollisionObjMoving::GetTransform(btTransform& Out) const
{
	n_assert_dbg(pBtCollObj);

	btMotionState* pMotionState = ((btRigidBody*)pBtCollObj)->getMotionState();
	if (pMotionState)
	{
		// get from motion state cache
	}
	else CCollisionObj::GetTransform(Out);
}
//---------------------------------------------------------------------

}