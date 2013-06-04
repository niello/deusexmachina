#include "RigidBody.h"

#include <Physics/BulletConv.h>
#include <Physics/PhysicsWorld.h>
#include <Physics/MotionStateDynamic.h>
#include <Data/Params.h>
#include <mathlib/bbox.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CRigidBody, Physics::CPhysicsObj);

bool CRigidBody::Init(const Data::CParams& Desc, const vector3& Offset)
{
	if (!CPhysicsObj::Init(Desc, Offset)) FAIL;

	float Mass = Desc.Get<float>(CStrID("Mass"), 1.f);

	CMotionStateDynamic* pMS = new CMotionStateDynamic;

	btVector3 Inertia;
	Shape->GetBtShape()->calculateLocalInertia(Mass, Inertia);
	btRigidBody::btRigidBodyConstructionInfo CI(Mass, pMS, Shape->GetBtShape(), Inertia);

	//!!!set friction and restitution!

	pBtCollObj = new btRigidBody(CI);
	pBtCollObj->setUserPointer(this);

	OK;
}
//---------------------------------------------------------------------

void CRigidBody::InternalTerm()
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

void CRigidBody::Term()
{
	InternalTerm();
	CPhysicsObj::Term();
}
//---------------------------------------------------------------------

bool CRigidBody::AttachToLevel(CPhysicsWorld& World)
{
	if (!CPhysicsObj::AttachToLevel(World)) FAIL;

	// Enforce offline transform update to be taken into account
	btRigidBody* pRB = (btRigidBody*)pBtCollObj;
	pRB->setMotionState(pRB->getMotionState());
	pWorld->GetBtWorld()->addRigidBody(pRB, Group, Mask);

	OK;
}
//---------------------------------------------------------------------

void CRigidBody::RemoveFromLevel()
{
	if (!pWorld || !pWorld->GetBtWorld()) return;
	pWorld->GetBtWorld()->removeRigidBody((btRigidBody*)pBtCollObj);
	CPhysicsObj::RemoveFromLevel();
}
//---------------------------------------------------------------------

void CRigidBody::SetTransform(const matrix44& Tfm)
{
	n_assert_dbg(pBtCollObj);

	CMotionStateDynamic* pMotionState = (CMotionStateDynamic*)((btRigidBody*)pBtCollObj)->getMotionState();
	if (pMotionState)
	{
		btTransform BtTfm = TfmToBtTfm(Tfm);
		pMotionState->Rotation = BtTfm.getRotation();
		pMotionState->Position = BtTfm.getOrigin();
		pMotionState->TfmChanged = true;

		//???why crashes?
		//pMotionState->setWorldTransform(TfmToBtTfm(Tfm)); //???optimize?

		pMotionState->Position.m_floats[0] += ShapeOffset.x;
		pMotionState->Position.m_floats[1] += ShapeOffset.y;
		pMotionState->Position.m_floats[2] += ShapeOffset.z;
	}
	else CPhysicsObj::SetTransform(Tfm);
}
//---------------------------------------------------------------------

void CRigidBody::GetTransform(vector3& OutPos, quaternion& OutRot) const
{
	n_assert_dbg(pBtCollObj);

	CMotionStateDynamic* pMotionState = (CMotionStateDynamic*)((btRigidBody*)pBtCollObj)->getMotionState();
	if (pMotionState)
	{
		OutRot = BtQuatToQuat(pMotionState->Rotation);
		OutPos = BtVectorToVector(pMotionState->Position) - ShapeOffset;
	}
	else CPhysicsObj::GetTransform(OutPos, OutRot);
}
//---------------------------------------------------------------------

void CRigidBody::GetTransform(btTransform& Out) const
{
	n_assert_dbg(pBtCollObj);

	CMotionStateDynamic* pMotionState = (CMotionStateDynamic*)((btRigidBody*)pBtCollObj)->getMotionState();
	if (pMotionState)
	{
		pMotionState->getWorldTransform(Out);

		Out.getOrigin().m_floats[0] -= ShapeOffset.x;
		Out.getOrigin().m_floats[1] -= ShapeOffset.y;
		Out.getOrigin().m_floats[2] -= ShapeOffset.z;
	}
	else CPhysicsObj::GetTransform(Out);
}
//---------------------------------------------------------------------

void CRigidBody::SetTransformChanged(bool Changed)
{
	CMotionStateDynamic* pMotionState = (CMotionStateDynamic*)((btRigidBody*)pBtCollObj)->getMotionState();
	if (pMotionState) pMotionState->TfmChanged = Changed;
}
//---------------------------------------------------------------------

bool CRigidBody::IsTransformChanged() const
{
	CMotionStateDynamic* pMotionState = (CMotionStateDynamic*)((btRigidBody*)pBtCollObj)->getMotionState();
	return pMotionState ? pMotionState->TfmChanged : true;
}
//---------------------------------------------------------------------

}