#pragma once
#ifndef __DEM_L1_TRIGGER_CONTACT_CB_H__
#define __DEM_L1_TRIGGER_CONTACT_CB_H__

#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

namespace Physics
{

class CTriggerContactCallback: public btCollisionWorld::ContactResultCallback
{
protected:

	btCollisionObject*					pSelf;
	CArray<const btCollisionObject*>&	CollResults;

public:

	CTriggerContactCallback(btCollisionObject* pMe, CArray<const btCollisionObject*>& Results, ushort Group, ushort Mask):
		pSelf(pMe), CollResults(Results)
	{
		m_collisionFilterGroup = Group;
		m_collisionFilterMask = Mask;
	}

	virtual btScalar addSingleResult(btManifoldPoint& cp,
									const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0,
									const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1)
	{
		// NB: objects may be duplicated
		CollResults.Add(colObj0Wrap->getCollisionObject() == pSelf ?
			colObj1Wrap->getCollisionObject() :
			colObj0Wrap->getCollisionObject());
		return 0;
	}
};

}

#endif