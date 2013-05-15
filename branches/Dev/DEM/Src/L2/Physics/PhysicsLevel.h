#pragma once
#ifndef __DEM_L2_PHYSICS_LEVEL_H__ //!!!to L1!
#define __DEM_L2_PHYSICS_LEVEL_H__

// The Physics level contains all the physics entities.
// Has a "point of interest" property which should be set to the point
// where the action happens (for instance where the player controlled
// character is at the moment). This is useful for huge levels where
// physics should only happen in an area around the player.

#include <Core/RefCounted.h>
#include <Physics/Joints/Joint.h>
#include <Physics/MaterialTable.h>
#include <util/HashMap.h>
#include <Debug/Profiler.h>

namespace Physics
{
typedef Ptr<class CEntity> PEntity;
class CShape;
class Ray;

class CPhysicsLevel: public Core::CRefCounted
{
	__DeclareClass(CPhysicsLevel);

protected:

	enum { MaxContacts = 16 };

	nTime					TimeToSim;
	nTime					StepSize;
	nArray<PEntity>			Entities;
	nArray<Ptr<CShape>>		Shapes;
	vector3					POI;
	vector3					Gravity;

	dWorldID				ODEWorldID;
	dSpaceID				ODECommonSpaceID;	// contains both the static and dynamic space
	dSpaceID				ODEStaticSpaceID;	// collide space for static geoms
	dSpaceID				ODEDynamicSpaceID;	// collide space for dynamic geoms
	dJointGroupID			ContactJointGroup;

	//???here?
	CHashMap<nTime>		CollisionSounds;

	PROFILER_DECLARE(profFrameBefore);
	PROFILER_DECLARE(profFrameAfter);
	PROFILER_DECLARE(profStepBefore);
	PROFILER_DECLARE(profStepAfter);
	PROFILER_DECLARE(profCollide);
	PROFILER_DECLARE(profStep);
	PROFILER_DECLARE(profJointGroupEmpty);

#ifdef DEM_STATS
	int statsNumSpaceCollideCalled;              // number of times dSpaceCollide has been invoked
	int statsNumNearCallbackCalled;              // number of times the near callback has been invoked
	int statsNumCollideCalled;                   // number of times the collide function has been invoked
	int statsNumCollided;                        // number of times two shapes have collided
	int statsNumSpaces;
	int statsNumShapes;
	int statsNumSteps;
#endif

	// ODE collision callback
	static void ODENearCallback(void* data, dGeomID o1, dGeomID o2);

public:

	CPhysicsLevel();
	virtual ~CPhysicsLevel();

	virtual void	Activate();
	virtual void	Deactivate();
	virtual void	Trigger();
	void			RenderDebug();

	void			AttachShape(CShape* pShape);
	void			RemoveShape(CShape* pShape);
	int				GetNumShapes() const { return Shapes.GetCount(); }
	CShape*			GetShapeAt(int Idx) const { return Shapes[Idx]; }

	void			AttachEntity(CEntity* pEnt);
	void			RemoveEntity(CEntity* pEnt);
	int				GetNumEntities() const { return Entities.GetCount(); }
	CEntity*		GetEntityAt(int Idx) const { return Entities[Idx]; }

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

__RegisterClassInFactory(CPhysicsLevel);

typedef Ptr<CPhysicsLevel> PPhysicsLevel;

}

#endif
