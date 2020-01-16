#include "PhysicsObject.h"
#include <Physics/BulletConv.h>
#include <Physics/PhysicsLevel.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{
RTTI_CLASS_IMPL(Physics::CPhysicsObject, Core::CObject);

CPhysicsObject::CPhysicsObject(CStrID CollisionGroupID, CStrID CollisionMaskID)
	: _CollisionGroupID(CollisionGroupID)
	, _CollisionMaskID(CollisionMaskID)
{
}
//---------------------------------------------------------------------

void CPhysicsObject::AttachToLevel(CPhysicsLevel& Level)
{
	if (_Level == &Level) return;

	if (_Level) RemoveFromLevel();

	_Level = &Level;
	AttachToLevelInternal();
}
//---------------------------------------------------------------------

void CPhysicsObject::RemoveFromLevel()
{
	if (!_Level) return;
	RemoveFromLevelInternal();
	_Level = nullptr;
}
//---------------------------------------------------------------------

// Returns end-of-tick AABB from the physics world
void CPhysicsObject::GetPhysicsAABB(CAABB& OutBox) const
{
	if (!_pBtObject) return;

	btVector3 Min, Max;
	if (_Level)
		_Level->GetBtWorld()->getBroadphase()->getAabb(_pBtObject->getBroadphaseHandle(), Min, Max);
	else
		_pBtObject->getCollisionShape()->getAabb(_pBtObject->getWorldTransform(), Min, Max);
	OutBox.Min = BtVectorToVector(Min);
	OutBox.Max = BtVectorToVector(Max);
}
//---------------------------------------------------------------------

const CCollisionShape* CPhysicsObject::GetCollisionShape() const
{
	return _pBtObject ? static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer()) : nullptr;
}
//---------------------------------------------------------------------

bool CPhysicsObject::IsActive() const
{
	return _pBtObject ? _pBtObject->isActive() : false;
}
//---------------------------------------------------------------------

}
