#include "CollisionObj.h"

#include <Physics/BulletConv.h>
#include <Physics/PhysicsWorld.h>
#include <mathlib/bbox.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CCollisionObj, Core::CRefCounted);

bool CCollisionObj::Init(CCollisionShape& CollShape, ushort Group, ushort Mask, const vector3& Offset)
{
	n_assert(pBtCollObj);
	Shape = &CollShape;
	pBtCollObj->setUserPointer(this);
	OK;
}
//---------------------------------------------------------------------

void CCollisionObj::Term()
{
	if (pWorld)
	{
		//!!!remove if added!
		pWorld = NULL;
	}

	if (pBtCollObj)
	{
		delete pBtCollObj;
		pBtCollObj = NULL;
	}

	Shape = NULL;
}
//---------------------------------------------------------------------

bool CCollisionObj::AttachToLevel(CPhysicsWorld& World)
{
	if (!World.GetBtWorld() || !pBtCollObj) FAIL;

	n_assert(!pWorld);
	pWorld = &World;
	//World.CollObjects.Append(this);

	OK;
}
//---------------------------------------------------------------------

void CCollisionObj::RemoveFromLevel()
{
	if (pWorld)
	{
		//World.CollObjects.RemoveByValue(this);
		pWorld = NULL;
	}
}
//---------------------------------------------------------------------

bool CCollisionObj::SetTransform(const matrix44& Tfm)
{
	btTransform BtTfm = TfmToBtTfm(Tfm);

	BtTfm.getOrigin().m_floats[0] += Offset.x;
	BtTfm.getOrigin().m_floats[1] += Offset.y;
	BtTfm.getOrigin().m_floats[2] += Offset.z;

	vector3 ShapeOffset;
	if (Shape->GetOffset(ShapeOffset))
	{
		BtTfm.getOrigin().m_floats[0] += ShapeOffset.x;
		BtTfm.getOrigin().m_floats[1] += ShapeOffset.y;
		BtTfm.getOrigin().m_floats[2] += ShapeOffset.z;
	}

	pBtCollObj->setWorldTransform(BtTfm);
	if (pWorld) pWorld->GetBtWorld()->updateSingleAabb(pBtCollObj); //???for kinematic too?

	OK;
}
//---------------------------------------------------------------------

void CCollisionObj::GetGlobalAABB(bbox3& OutBox) const
{
	btVector3 Min, Max;
	if (pWorld) pWorld->GetBtWorld()->getBroadphase()->getAabb(pBtCollObj->getBroadphaseHandle(), Min, Max);
	else pBtCollObj->getCollisionShape()->getAabb(pBtCollObj->getWorldTransform(), Min, Max);
	OutBox.vmin = BtVectorToVector(Min);
	OutBox.vmax = BtVectorToVector(Max);
}
//---------------------------------------------------------------------

}
