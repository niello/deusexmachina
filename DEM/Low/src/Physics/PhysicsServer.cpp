#include "PhysicsServer.h"

#include <Physics/BulletConv.h>
#include <Physics/CollisionShape.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>

namespace Physics
{
__ImplementSingleton(Physics::CPhysicsServer);

CPhysicsServer::CPhysicsServer()
{
	__ConstructSingleton;

	/* Bullet predefined collision filters as of v2.81 SDK:
	DefaultFilter = 1,
	StaticFilter = 2,
	KinematicFilter = 4,
	DebrisFilter = 8,
	SensorTrigger = 16,
	CharacterFilter = 32,
	AllFilter = -1
	*/

	// We can use not all Bullet flags
	CollisionGroups.SetAlias(CStrID("DefaultFilter"), "Default");

	U16 PickMask = CollisionGroups.GetMask("MousePick");
	U16 PickTargetMask = CollisionGroups.GetMask("MousePickTarget");
	CollisionGroups.SetAlias(CStrID("All"), ~PickTargetMask);
	CollisionGroups.SetAlias(CStrID("AllNoPick"), (~PickTargetMask) & (~PickMask));
}
//---------------------------------------------------------------------

CPhysicsServer::~CPhysicsServer()
{
	__DestructSingleton;
}
//---------------------------------------------------------------------

PCollisionShape CPhysicsServer::CreateBoxShape(const vector3& Size)
{
	PCollisionShape Shape = n_new(CCollisionShape);
	btBoxShape* pBtShape = new btBoxShape(VectorToBtVector(Size * 0.5f));
	Shape->Setup(pBtShape);
	return Shape;
}
//---------------------------------------------------------------------

PCollisionShape CPhysicsServer::CreateSphereShape(float Radius)
{
	PCollisionShape Shape = n_new(CCollisionShape);
	btSphereShape* pBtShape = new btSphereShape(Radius);
	Shape->Setup(pBtShape);
	return Shape;
}
//---------------------------------------------------------------------

// Vertical capsule (around Y axis)
PCollisionShape CPhysicsServer::CreateCapsuleShape(float Radius, float Height)
{
	PCollisionShape Shape = n_new(CCollisionShape);
	btCapsuleShape* pBtShape = new btCapsuleShape(Radius, Height);
	Shape->Setup(pBtShape);
	return Shape;
}
//---------------------------------------------------------------------

}