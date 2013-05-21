#pragma once
#ifndef __DEM_L1_PHYSICS_WORLD_H__ //!!!to L1!
#define __DEM_L1_PHYSICS_WORLD_H__

#include <Core/RefCounted.h>

//#include <LinearMath/btScalar.h>

#include <Physics/Joints/Joint.h>
#include <Physics/MaterialTable.h>
#include <util/HashMap.h>
#include <Debug/Profiler.h>

// Physics world represents a space where physics bodies and shapes live.

class btDiscreteDynamicsWorld; //???class btDynamicsWorld;

namespace Physics
{
typedef Ptr<class CEntity> PEntity;
class CShape;

class CPhysWorld: public Core::CRefCounted
{
	__DeclareClassNoFactory;

protected:

	btDiscreteDynamicsWorld*	pBtDynWorld;


//!!!OLD!
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

	CPhysWorld();
	virtual ~CPhysWorld();

	bool	Init(const bbox3& Bounds);
	void	Term();
	void	Trigger(float FrameTime);
	void	RenderDebug();

	void			AttachShape(CShape* pShape);
	void			RemoveShape(CShape* pShape);
	int				GetNumShapes() const { return Shapes.GetCount(); }
	CShape*			GetShapeAt(int Idx) const { return Shapes[Idx]; }

	void			AttachEntity(CEntity* pEnt);
	void			RemoveEntity(CEntity* pEnt);
	int				GetNumEntities() const { return Entities.GetCount(); }
	CEntity*		GetEntityAt(int Idx) const { return Entities[Idx]; }

	bool					RayCheck(const vector3& Pos, const vector3& Dir);
	const CContactPoint*	GetClosestContactAlongRay(const vector3& Pos, const vector3& Dir, DWORD SelfPhysID = -1);

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

typedef Ptr<CPhysWorld> PPhysicsLevel;

}

#endif
