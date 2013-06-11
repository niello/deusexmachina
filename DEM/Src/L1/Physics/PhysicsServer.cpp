#include "PhysicsServer.h"

#include <Physics/BulletConv.h>
#include <Physics/PhysicsDebugDraw.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>

namespace Physics
{
__ImplementClass(Physics::CPhysicsServer, 'PHSR', Core::CRefCounted);
__ImplementSingleton(Physics::CPhysicsServer);

CPhysicsServer::CPhysicsServer(): _IsOpen(false)
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

	ushort PickMask = CollisionGroups.GetMask("MousePick");
	ushort PickTargetMask = CollisionGroups.GetMask("MousePickTarget");
	CollisionGroups.SetAlias(CStrID("All"), ~PickTargetMask);
	CollisionGroups.SetAlias(CStrID("AllNoPick"), (~PickTargetMask) & (~PickMask));
}
//---------------------------------------------------------------------

CPhysicsServer::~CPhysicsServer()
{
	n_assert(!_IsOpen);
	__DestructSingleton;
}
//---------------------------------------------------------------------

bool CPhysicsServer::Open()
{
	n_assert(!_IsOpen);

	pDD = n_new(CPhysicsDebugDraw);
	pDD->setDebugMode(CPhysicsDebugDraw::DBG_DrawAabb | CPhysicsDebugDraw::DBG_DrawWireframe | CPhysicsDebugDraw::DBG_FastWireframe);

	_IsOpen = true;
	OK;
}
//---------------------------------------------------------------------

void CPhysicsServer::Close()
{
	n_assert(_IsOpen);

	if (pDD)
	{
		n_delete(pDD);
		pDD = NULL;
	}

	_IsOpen = false;
}
//---------------------------------------------------------------------

PCollisionShape CPhysicsServer::CreateBoxShape(const vector3& Size, CStrID UID)
{
	PCollisionShape Shape = CollisionShapeMgr.CreateTypedResource(UID);
	btBoxShape* pBtShape = new btBoxShape(VectorToBtVector(Size * 0.5f));
	Shape->Setup(pBtShape);
	return Shape;
}
//---------------------------------------------------------------------

PCollisionShape CPhysicsServer::CreateSphereShape(float Radius, CStrID UID)
{
	PCollisionShape Shape = CollisionShapeMgr.CreateTypedResource(UID);
	btSphereShape* pBtShape = new btSphereShape(Radius);
	Shape->Setup(pBtShape);
	return Shape;
}
//---------------------------------------------------------------------

// Vertical capsule (around Y axis)
PCollisionShape CPhysicsServer::CreateCapsuleShape(float Radius, float Height, CStrID UID)
{
	PCollisionShape Shape = CollisionShapeMgr.CreateTypedResource(UID);
	btCapsuleShape* pBtShape = new btCapsuleShape(Radius, Height);
	Shape->Setup(pBtShape);
	return Shape;
}
//---------------------------------------------------------------------

}