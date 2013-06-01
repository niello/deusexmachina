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

	CPhysicsObj::Term();

	if (pMS) delete pMS;
}
//---------------------------------------------------------------------

bool CCollisionObjMoving::AttachToLevel(CPhysicsWorld& World)
{
	if (!CPhysicsObj::AttachToLevel(World)) FAIL;
	pWorld->GetBtWorld()->addRigidBody((btRigidBody*)pBtCollObj, Group, Mask);
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