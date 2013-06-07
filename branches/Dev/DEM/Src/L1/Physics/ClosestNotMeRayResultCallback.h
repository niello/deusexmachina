#pragma once
#ifndef __DEM_L1_CNM_RAY_CB_H__
#define __DEM_L1_CNM_RAY_CB_H__

#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

namespace Physics
{

class CClosestNotMeRayResultCallback: public btCollisionWorld::ClosestRayResultCallback
{
protected:

	btCollisionObject* pSelf;

public:

	CClosestNotMeRayResultCallback(btCollisionObject* pMe):
	  ClosestRayResultCallback(btVector3(0.f, 0.f, 0.f), btVector3(0.f, 0.f, 0.f)), pSelf(pMe)
	{
	}

	CClosestNotMeRayResultCallback(const btVector3& From, const btVector3& To, btCollisionObject* pMe):
	  ClosestRayResultCallback(From, To), pSelf(pMe)
	{
	}

	virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
	{
		if (rayResult.m_collisionObject == pSelf) return 1.f;
		return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
	}
};

}

#endif