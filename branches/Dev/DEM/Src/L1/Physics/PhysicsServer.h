#pragma once
#ifndef __DEM_L1_PHYSICS_SERVER_H__
#define __DEM_L1_PHYSICS_SERVER_H__

#include <Core/Singleton.h>
#include <Resources/ResourceManager.h>
#include <Physics/CollisionShape.h>

// Physics server stores shape manager, which manages shared collision shape resources

namespace Physics
{
#define PhysicsSrv Physics::CPhysicsServer::Instance()

class CPhysicsServer: public Core::CRefCounted
{
	__DeclareClass(CPhysicsServer);
	__DeclareSingleton(CPhysicsServer);

protected:

	bool _IsOpen;

public:

	Resources::CResourceManager<CCollisionShape>	CollShapeMgr;

	CPhysicsServer();
	~CPhysicsServer();

	bool	Open();
	void	Close();
	bool	IsOpen() const { return _IsOpen; }
};

}

#endif
