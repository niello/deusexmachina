#pragma once
#ifndef __DEM_L1_PHYSICS_SERVER_H__
#define __DEM_L1_PHYSICS_SERVER_H__

#include <Core/Object.h>
#include <Data/Singleton.h>
#include <Data/DynamicEnum.h>
#include <Math/Vector3.h>

// Physics server stores shape manager, which manages shared collision shape resources,
// shared debug drawer implementation and other physics services.

namespace Physics
{
typedef Ptr<class CCollisionShape> PCollisionShape;

#define PhysicsSrv Physics::CPhysicsServer::Instance()

class CPhysicsServer: public Core::CObject
{
	__DeclareSingleton(CPhysicsServer);

public:

	Data::CDynamicEnum16	CollisionGroups;

	CPhysicsServer();
	~CPhysicsServer();

	PCollisionShape		CreateBoxShape(const vector3& Size);
	PCollisionShape		CreateSphereShape(float Radius);
	PCollisionShape		CreateCapsuleShape(float Radius, float Height);
};

}

#endif
