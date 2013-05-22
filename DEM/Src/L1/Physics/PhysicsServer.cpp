#include "PhysicsServer.h"

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
	_IsOpen = true;
	OK;
}
//---------------------------------------------------------------------

void CPhysicsServer::Close()
{
	n_assert(_IsOpen);
	_IsOpen = false;
}
//---------------------------------------------------------------------

}