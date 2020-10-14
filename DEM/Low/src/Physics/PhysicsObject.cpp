#include "PhysicsObject.h"
#include <Physics/BulletConv.h>
#include <Physics/PhysicsLevel.h>
#include <Physics/HeightfieldShape.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{

CPhysicsObject::CPhysicsObject(CStrID CollisionGroupID, CStrID CollisionMaskID)
	: _CollisionGroupID(CollisionGroupID)
	, _CollisionMaskID(CollisionMaskID)
{
}
//---------------------------------------------------------------------

void CPhysicsObject::SetupInternalObject(btCollisionObject* pBtObject, const CCollisionShape& Shape, const CPhysicsMaterial& Material)
{
	_pBtObject = pBtObject;
	if (_pBtObject->getCollisionShape() != Shape.GetBulletShape())
		_pBtObject->setCollisionShape(Shape.GetBulletShape());
	_pBtObject->setFriction(Material.Friction);
	_pBtObject->setRollingFriction(Material.RollingFriction);
	_pBtObject->setRestitution(1.f - Material.Bounciness);
	_pBtObject->setUserPointer(this);

	// As of Bullet v2.89 SDK, debug drawer tries to draw each heightfield triangle wireframe,
	// so we disable debug drawing of terrain at all
	// TODO: terrain is most probably a static collider, not movable!
	if (Shape.IsA<CHeightfieldShape>())
		_pBtObject->setCollisionFlags(_pBtObject->getCollisionFlags() | btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT);
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

bool CPhysicsObject::IsAlwaysActive() const
{
	return _pBtObject ? (_pBtObject->getActivationState() == DISABLE_DEACTIVATION) : false;
}
//---------------------------------------------------------------------

}
