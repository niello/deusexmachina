#include "PhysicsServer.h"

#include <Physics/PhysicsDebugDraw.h>

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
	pDD->setDebugMode(CPhysicsDebugDraw::DBG_DrawAabb);

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

}