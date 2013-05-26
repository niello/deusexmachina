#pragma once
#ifndef __DEM_L1_PHYSICS_WORLD_OLD_H__ //!!!to L1!
#define __DEM_L1_PHYSICS_WORLD_OLD_H__

#include <Core/RefCounted.h>
#include <Physics/Joints/Joint.h>
#include <Physics/MaterialTable.h>
#include <util/HashMap.h>
#include <Debug/Profiler.h>

// Physics world represents a space where physics bodies and shapes live.

namespace Physics
{
typedef Ptr<class CEntity> PEntity;
class CShape;

class CPhysWorldOld: public Core::CRefCounted
{
	__DeclareClassNoFactory;

protected:

	enum { MaxContacts = 16 };

	nTime					TimeToSim;
	nTime					StepSize;
	nArray<PEntity>			Entities;
	nArray<Ptr<CShape>>		Shapes;
	vector3					POI;
	vector3					Gravity;

	nArray<CContactPoint>	Contacts;
	dGeomID					ODERayID;

	dWorldID				ODEWorldID;
	dSpaceID				ODECommonSpaceID;	// contains both the static and dynamic space
	dSpaceID				ODEStaticSpaceID;	// collide space for static geoms
	dSpaceID				ODEDynamicSpaceID;	// collide space for dynamic geoms
	dJointGroupID			ContactJointGroup;

	//???here?
	CHashMap<nTime>		CollisionSounds;

	// ODE collision callbacks
	static void ODENearCallback(void* data, dGeomID o1, dGeomID o2);
	static void ODERayCallback(void* data, dGeomID o1, dGeomID o2);

public:

	CPhysWorldOld();
	virtual ~CPhysWorldOld();

	bool	Init(const bbox3& Bounds);
	void	Term();
	void	Trigger(float FrameTime);
	void	RenderDebug();

	void			AttachShape(CShape* pShape);
	void			RemoveShape(CShape* pShape);
	void			AttachEntity(CEntity* pEnt);
	void			RemoveEntity(CEntity* pEnt);

	bool					RayTest(const vector3& Pos, const vector3& Dir);
	const CContactPoint*	GetClosestRayContact(const vector3& Pos, const vector3& Dir, DWORD SelfPhysID = -1);

	void			SetStepSize(nTime t) { StepSize = t; }
	nTime			GetStepSize() const { return StepSize; }
	void			SetPointOfInterest(const vector3& NewPOI) { POI = NewPOI; }
	const vector3&	GetPointOfInterest() const { return POI; }
	void			SetGravity(const vector3& NewGravity);
	const vector3&	GetGravity() const { return Gravity; }
	dWorldID		GetODEWorldID() const { return ODEWorldID; }
	dSpaceID		GetODEStaticSpaceID() const { return ODEStaticSpaceID; }
	dSpaceID		GetODEDynamicSpaceID() const { return ODEDynamicSpaceID; }
	dSpaceID		GetODECommonSpaceID() const { return ODECommonSpaceID; }
};

typedef Ptr<CPhysWorldOld> PPhysWorldOld;

}

#endif
