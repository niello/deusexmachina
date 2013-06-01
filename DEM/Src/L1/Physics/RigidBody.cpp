#include "RigidBody.h"

#include <Physics/BulletConv.h>
#include <Physics/PhysicsWorld.h>
#include <Physics/MotionStateFromNode.h>
#include <Data/Params.h>
#include <mathlib/bbox.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CRigidBody, Physics::CCollisionObj);

bool CRigidBody::Init(const Data::CParams& Desc, const vector3& Offset)
{
	if (!CCollisionObj::Init(Desc, Offset)) FAIL;

	float Mass = Desc.Get<float>(CStrID("Mass"), 1.f);

	//!!!motion state!

	btVector3 Inertia;
	Shape->GetBtShape()->calculateLocalInertia(Mass, Inertia);
	btRigidBody::btRigidBodyConstructionInfo CI(Mass, NULL, Shape->GetBtShape(), Inertia);
	//???pass material and set friction, restitution etc?

	pBtCollObj = new btRigidBody(CI);
	pBtCollObj->setUserPointer(this);

	OK;
}
//---------------------------------------------------------------------

void CRigidBody::Term()
{
	btMotionState* pMS = pBtCollObj ? ((btRigidBody*)pBtCollObj)->getMotionState() : NULL;

	CCollisionObj::Term();

	if (pMS) delete pMS;
}
//---------------------------------------------------------------------

bool CRigidBody::AttachToLevel(CPhysicsWorld& World)
{
	if (!CCollisionObj::AttachToLevel(World)) FAIL;
	pWorld->GetBtWorld()->addRigidBody((btRigidBody*)pBtCollObj, Group, Mask);
	OK;
}
//---------------------------------------------------------------------

void CRigidBody::RemoveFromLevel()
{
	if (!pWorld) return;
	n_assert(pWorld->GetBtWorld());
	pWorld->GetBtWorld()->removeRigidBody((btRigidBody*)pBtCollObj);
	CCollisionObj::RemoveFromLevel();
}
//---------------------------------------------------------------------

}