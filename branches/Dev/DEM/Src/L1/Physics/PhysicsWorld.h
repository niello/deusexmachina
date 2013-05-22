#pragma once
#ifndef __DEM_L1_PHYSICS_WORLD_H__
#define __DEM_L1_PHYSICS_WORLD_H__

#include <Core/RefCounted.h>

// Physics world represents a space where physics bodies and collision objects live.

class bbox3;
class btDiscreteDynamicsWorld;

namespace Physics
{

class CPhysicsWorld: public Core::CRefCounted
{
	__DeclareClassNoFactory;

protected:

	btDiscreteDynamicsWorld*	pBtDynWorld;
	float						StepTime;

public:

	CPhysicsWorld(): pBtDynWorld(NULL), StepTime(1.f / 60.f) {}
	~CPhysicsWorld() { Term(); }

	bool	Init(const bbox3& Bounds);
	void	Term();
	void	Trigger(float FrameTime);
	void	RenderDebug();

	//!!!can set filter group and mask!
	bool	GetClosestRayContact(const vector3& Start, const vector3& End) const;
	DWORD	GetAllRayContacts(const vector3& Start, const vector3& End) const;

	//int GetAllShapeContacts(PCollisionShape Shape, const CFilterSet& ExcludeSet, nArray<PEntity>& Result);

	void	SetStepTime(float Secs) { n_assert(Secs > 0.f); StepTime = Secs; }
	float	GetStepTime() const { return StepTime; }
	void	SetGravity(const vector3& NewGravity);
	void	GetGravity(vector3& Out) const;

	//void			SetPointOfInterest(const vector3& NewPOI) { POI = NewPOI; }
	//const vector3&	GetPointOfInterest() const { return POI; }
};

typedef Ptr<CPhysicsWorld> PPhysicsWorld;

// Not too elegant, but reduces the length of the full "Physics::CPhysicsWorld" name
typedef CPhysicsWorld CPhysWorld;
typedef PPhysicsWorld PPhysWorld;

}

#endif
