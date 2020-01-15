#include "PhysicsObject.h"
#include <Physics/BulletConv.h>
#include <Physics/PhysicsLevel.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
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

CPhysicsObject::~CPhysicsObject() = default;
//---------------------------------------------------------------------

bool CPhysicsObject::IsActive() const
{
	return _pBtObject ? _pBtObject->isActive() : false;
}
//---------------------------------------------------------------------

// Returns end-of-tick AABB from the physics world
void CPhysicsObject::GetPhysicsAABB(CAABB& OutBox) const
{
	if (!_pBtObject) return;

	btVector3 Min, Max;
	_Level->GetBtWorld()->getBroadphase()->getAabb(_pBtObject->getBroadphaseHandle(), Min, Max);
	//pBtCollObj->getCollisionShape()->getAabb(pBtCollObj->getWorldTransform(), Min, Max);
	OutBox.Min = BtVectorToVector(Min);
	OutBox.Max = BtVectorToVector(Max);
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
