#include "RigidBody.h"

#include <Physics/BulletConv.h>
#include <Physics/PhysicsWorld.h>
#include <Physics/MotionStateDynamic.h>
#include <Data/Params.h>
#include <Math/AABB.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CRigidBody, Physics::CPhysicsObject);

bool CRigidBody::Init(const Data::CParams& Desc, const vector3& Offset)
{
	if (!CPhysicsObject::Init(Desc, Offset)) FAIL;
	return InternalInit(Desc.Get<float>(CStrID("Mass"), 1.f));
}
//---------------------------------------------------------------------

bool CRigidBody::Init(CCollisionShape& CollShape, float BodyMass, U16 CollGroup, U16 CollMask, const vector3& Offset)
{
	if (!CPhysicsObject::Init(CollShape, CollGroup, CollMask, Offset)) FAIL;
	return InternalInit(BodyMass);
}
//---------------------------------------------------------------------

void CRigidBody::Term()
{
	InternalTerm();
	CPhysicsObject::Term();
}
//---------------------------------------------------------------------

bool CRigidBody::InternalInit(float BodyMass)
{
	Mass = BodyMass;
	btVector3 Inertia;
	Shape->GetBtShape()->calculateLocalInertia(Mass, Inertia);

	CMotionStateDynamic* pMS = new CMotionStateDynamic;
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

bool CRigidBody::AttachToLevel(CPhysicsWorld& World)
{
	if (!CPhysicsObject::AttachToLevel(World)) FAIL;

	// Enforce offline transform update to be taken into account
	btRigidBody* pRB = (btRigidBody*)pBtCollObj;
	pRB->setMotionState(pRB->getMotionState());
	pWorld->GetBtWorld()->addRigidBody(pRB, Group, Mask);

	// Sometimes we read tfm from motion state before any real tick is performed.
	// For this case make that tfm up-to-date.
	pRB->setInterpolationWorldTransform(pRB->getWorldTransform());

	OK;
}
//---------------------------------------------------------------------

void CRigidBody::RemoveFromLevel()
{
	if (!pWorld || !pWorld->GetBtWorld()) return;
	pWorld->GetBtWorld()->removeRigidBody((btRigidBody*)pBtCollObj);
	CPhysicsObject::RemoveFromLevel();
}
//---------------------------------------------------------------------

void CRigidBody::SetTransform(const matrix44& Tfm)
{
	n_assert_dbg(pBtCollObj);

	CMotionStateDynamic* pMotionState = (CMotionStateDynamic*)((btRigidBody*)pBtCollObj)->getMotionState();
	if (pMotionState)
	{
		btTransform BtTfm = TfmToBtTfm(Tfm);
		BtTfm.getOrigin() = BtTfm * VectorToBtVector(ShapeOffset);
		pMotionState->setWorldTransform(BtTfm);
	}
	if (!pMotionState || pWorld) CPhysicsObject::SetTransform(Tfm);
}
//---------------------------------------------------------------------

void CRigidBody::GetTransform(vector3& OutPos, quaternion& OutRot) const
{
	n_assert_dbg(pBtCollObj);

	CMotionStateDynamic* pMotionState = (CMotionStateDynamic*)((btRigidBody*)pBtCollObj)->getMotionState();
	if (pMotionState)
	{
		OutRot = BtQuatToQuat(pMotionState->Rotation);
		OutPos = BtVectorToVector(pMotionState->Position) - OutRot.rotate(ShapeOffset);
	}
	else CPhysicsObject::GetTransform(OutPos, OutRot);
}
//---------------------------------------------------------------------

void CRigidBody::GetTransform(btTransform& Out) const
{
	n_assert_dbg(pBtCollObj);

	CMotionStateDynamic* pMotionState = (CMotionStateDynamic*)((btRigidBody*)pBtCollObj)->getMotionState();
	if (pMotionState)
	{
		pMotionState->getWorldTransform(Out);
		Out.getOrigin() = Out * VectorToBtVector(-ShapeOffset);
	}
	else CPhysicsObject::GetTransform(Out);
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

float CRigidBody::GetInvMass() const
{
	return ((btRigidBody*)pBtCollObj)->getInvMass();
}
//---------------------------------------------------------------------

bool CRigidBody::IsActive() const
{
	int ActState = ((btRigidBody*)pBtCollObj)->getActivationState();
	return ActState == DISABLE_DEACTIVATION || ActState == ACTIVE_TAG;
}
//---------------------------------------------------------------------

bool CRigidBody::IsAlwaysActive() const
{
	return ((btRigidBody*)pBtCollObj)->getActivationState() == DISABLE_DEACTIVATION;
}
//---------------------------------------------------------------------

bool CRigidBody::IsAlwaysInactive() const
{
	return ((btRigidBody*)pBtCollObj)->getActivationState() == DISABLE_SIMULATION;
}
//---------------------------------------------------------------------

void CRigidBody::MakeActive()
{
	((btRigidBody*)pBtCollObj)->forceActivationState(ACTIVE_TAG);
}
//---------------------------------------------------------------------

void CRigidBody::MakeAlwaysActive()
{
	((btRigidBody*)pBtCollObj)->forceActivationState(DISABLE_DEACTIVATION);
}
//---------------------------------------------------------------------

void CRigidBody::MakeAlwaysInactive()
{
	((btRigidBody*)pBtCollObj)->forceActivationState(DISABLE_SIMULATION);
}
//---------------------------------------------------------------------

}