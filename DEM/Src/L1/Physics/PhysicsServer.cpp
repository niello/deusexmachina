#include "PhysicsServer.h"

#include <Physics/PhysicsDebugDraw.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>

namespace Physics
{
__ImplementClass(Physics::CPhysicsServer, 'PHSR', Core::CRefCounted);
__ImplementSingleton(Physics::CPhysicsServer);

CPhysicsServer::CPhysicsServer(): _IsOpen(false)
{
	__ConstructSingleton;
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
	pDD->setDebugMode(CPhysicsDebugDraw::DBG_DrawAabb); // | CPhysicsDebugDraw::DBG_DrawWireframe);

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

// Vertical capsule (around Y axis)
PCollisionShape CPhysicsServer::CreateCapsuleShape(float Radius, float Height, CStrID UID)
{
	PCollisionShape Shape = CollShapeMgr.CreateTypedResource(UID);
	btCapsuleShape* pBtShape = new btCapsuleShape(Radius, Height);
	Shape->Setup(pBtShape);
	return Shape;
}
//---------------------------------------------------------------------

}