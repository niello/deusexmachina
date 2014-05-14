#pragma once
#ifndef __DEM_L1_PHYSICS_SERVER_H__
#define __DEM_L1_PHYSICS_SERVER_H__

#include <Data/Singleton.h>
#include <Resources/ResourceManager.h>
#include <Physics/CollisionShape.h>
#include <Data/DynamicEnum.h>

// Physics server stores shape manager, which manages shared collision shape resources,
// shared debug drawer implementation and other physics services.

namespace Physics
{
class CPhysicsDebugDraw;

#define PhysicsSrv Physics::CPhysicsServer::Instance()

class CPhysicsServer: public Core::CObject
{
	__DeclareClass(CPhysicsServer);
	__DeclareSingleton(CPhysicsServer);

protected:

	bool					_IsOpen;
	CPhysicsDebugDraw*		pDD;

public:

	Data::CDynamicEnum16							CollisionGroups;
	Resources::CResourceManager<CCollisionShape>	CollisionShapeMgr;

	CPhysicsServer();
	~CPhysicsServer();

	bool				Open();
	void				Close();
	bool				IsOpen() const { return _IsOpen; }

	PCollisionShape		CreateBoxShape(const vector3& Size, CStrID UID = CStrID::Empty);
	PCollisionShape		CreateSphereShape(float Radius, CStrID UID = CStrID::Empty);
	PCollisionShape		CreateCapsuleShape(float Radius, float Height, CStrID UID = CStrID::Empty);

	CPhysicsDebugDraw*	GetDebugDrawer() { return pDD; }
};

}

#endif
