#pragma once
#ifndef __DEM_L1_PHYSICS_WORLD_H__
#define __DEM_L1_PHYSICS_WORLD_H__

#include <Physics/CollisionObject.h>

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
	nArray<PCollisionObject>	CollObjects;

public:

	CPhysicsWorld(): pBtDynWorld(NULL), StepTime(1.f / 60.f) {}
	~CPhysicsWorld();

	bool	Init(const bbox3& Bounds);
	void	Term();
	void	Trigger(float FrameTime);
	void	RenderDebug();

	bool	AddCollisionObject(CCollisionObject& Obj, const matrix44& Tfm, ushort Group, ushort Mask);
	void	RemoveCollisionObject(CCollisionObject& Obj);

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

	btDiscreteDynamicsWorld* GetBtWorld() const { return pBtDynWorld; }
};

typedef Ptr<CPhysicsWorld> PPhysicsWorld;

}

#endif
