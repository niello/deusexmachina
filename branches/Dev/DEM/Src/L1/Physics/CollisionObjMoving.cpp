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
__ImplementClassNoFactory(Physics::CCollisionObjMoving, Scene::CNodeAttribute);

bool CCollisionObjMoving::Init(CCollisionShape& CollShape, ushort Group, ushort Mask, const vector3& Offset)
{
	n_assert(!pWorld && CollShape.IsLoaded());

	btRigidBody::btRigidBodyConstructionInfo CI(0.f, NULL, CollShape.GetBtShape());
	//???pass material and set friction, restitution, dampings etc?

	pBtCollObj = new btRigidBody(CI);
	pBtCollObj->setCollisionFlags(pBtCollObj->getCollisionFlags() | btRigidBody::CF_KINEMATIC_OBJECT);

	//???or activate only when tfm changes?
	pBtCollObj->setActivationState(DISABLE_DEACTIVATION);

	return CCollisionObj::Init(CollShape, Group, Mask, Offset);
}
//---------------------------------------------------------------------

void CCollisionObjMoving::Term()
{
	btMotionState* pMS = pBtCollObj ? ((btRigidBody*)pBtCollObj)->getMotionState() : NULL;

	CCollisionObj::Term();

	if (pMS) delete pMS;
}
//---------------------------------------------------------------------

void CCollisionObjMoving::SetNode(Scene::CSceneNode& Node)
{
	pNode = &Node;
	if (pWorld)
	{
		//???!!!offset to motion state?!
		btMotionState* pMotionState = ((btRigidBody*)pBtCollObj)->getMotionState();
		if (pMotionState) ((CMotionStateFromNode*)pMotionState)->SetNode(*pNode);
		else ((btRigidBody*)pBtCollObj)->setMotionState(new CMotionStateFromNode(*pNode));
	}
}
//---------------------------------------------------------------------

bool CCollisionObjMoving::AttachToLevel(CPhysicsWorld& World)
{
	if (!World.GetBtWorld() || !pBtCollObj || !pNode) FAIL;

	n_assert(!pWorld);
	pWorld = &World;
	//World.CollObjects.Append(this);

	SetNode(*pNode);
	pWorld->GetBtWorld()->addRigidBody((btRigidBody*)pBtCollObj, Group, Mask);

	OK;
}
//---------------------------------------------------------------------

void CCollisionObjMoving::RemoveFromLevel()
{
	if (!pWorld) return;
	n_assert(pWorld->GetBtWorld());

	pWorld->GetBtWorld()->removeRigidBody((btRigidBody*)pBtCollObj);

	pWorld = NULL;
	//World.CollObjects.RemoveByValue(this);
}
//---------------------------------------------------------------------

void CCollisionObjMoving::GetMotionStateAABB(bbox3& OutBox) const
{
	if (!pBtCollObj || !((btRigidBody*)pBtCollObj)->getMotionState()) return;

	const Scene::CSceneNode* pNode = ((CMotionStateFromNode*)((btRigidBody*)pBtCollObj)->getMotionState())->GetNode();
	btVector3 Min, Max;
	//!!!???take offset into account?!
	pBtCollObj->getCollisionShape()->getAabb(TfmToBtTfm(pNode->GetWorldMatrix()), Min, Max);
	OutBox.vmin = BtVectorToVector(Min);
	OutBox.vmax = BtVectorToVector(Max);
}
//---------------------------------------------------------------------

}