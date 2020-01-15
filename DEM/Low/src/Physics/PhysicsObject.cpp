#include "PhysicsObject.h"
#include <Physics/PhysicsLevel.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>
#include <BulletCollision/BroadphaseCollision/btBroadphaseProxy.h>

namespace Physics
{
RTTI_CLASS_IMPL(Physics::CPhysicsObject, Core::CObject);

CPhysicsObject::CPhysicsObject(CPhysicsLevel& Level)
	: _Level(&Level)
{
}
//---------------------------------------------------------------------

CPhysicsObject::~CPhysicsObject()
{
}
//---------------------------------------------------------------------

bool CPhysicsObject::IsActive() const
{
	return _pBtObject->isActive();
}
//---------------------------------------------------------------------

void CPhysicsObject::SetTransform(const matrix44& Tfm)
{
	/*
	n_assert_dbg(pBtCollObj);

	btTransform BtTfm = TfmToBtTfm(Tfm);
	BtTfm.getOrigin() = BtTfm * VectorToBtVector(ShapeOffset);

	pBtCollObj->setWorldTransform(BtTfm);
	if (pWorld) pWorld->GetBtWorld()->updateSingleAabb(pBtCollObj);
	*/
}
//---------------------------------------------------------------------

void CPhysicsObject::GetTransform(vector3& OutPos, quaternion& OutRot) const
{
/*
btTransform Tfm;
Tfm = pBtCollObj->getWorldTransform();
Tfm.getOrigin() = Tfm * VectorToBtVector(-ShapeOffset);
OutRot = BtQuatToQuat(Tfm.getRotation());
	OutPos = BtVectorToVector(Tfm.getOrigin());
	*/
}
//---------------------------------------------------------------------

// If possible, returns interpolated AABB from motion state. It matches the graphics representation.
void CPhysicsObject::GetGlobalAABB(CAABB& OutBox) const
{
/*
btTransform Tfm;
Tfm = pBtCollObj->getWorldTransform();
Tfm.getOrigin() = Tfm * VectorToBtVector(-ShapeOffset);

	btVector3 Min, Max;
	pBtCollObj->getCollisionShape()->getAabb(Tfm, Min, Max);
	OutBox.Min = BtVectorToVector(Min);
	OutBox.Max = BtVectorToVector(Max);
	*/
}
//---------------------------------------------------------------------

// Returns AABB from the physics world
void CPhysicsObject::GetPhysicsAABB(CAABB& OutBox) const
{
/*
n_assert_dbg(pBtCollObj);

	btVector3 Min, Max;
	pWorld->GetBtWorld()->getBroadphase()->getAabb(pBtCollObj->getBroadphaseHandle(), Min, Max);
	//pBtCollObj->getCollisionShape()->getAabb(pBtCollObj->getWorldTransform(), Min, Max);
	OutBox.Min = BtVectorToVector(Min);
	OutBox.Max = BtVectorToVector(Max);
	*/
}
//---------------------------------------------------------------------

const CCollisionShape* CPhysicsObject::GetCollisionShape() const
{
	return _pBtObject ? static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer()) : nullptr;
}
//---------------------------------------------------------------------

U16 CPhysicsObject::GetCollisionGroup() const
{
	return _pBtObject ? _pBtObject->getBroadphaseHandle()->m_collisionFilterGroup : 0;
}
//---------------------------------------------------------------------

U16 CPhysicsObject::GetCollisionMask() const
{
	return _pBtObject ? _pBtObject->getBroadphaseHandle()->m_collisionFilterMask : 0;
}
//---------------------------------------------------------------------

}
