#pragma once
#ifndef __DEM_L1_PHYSICS_LEVEL_H__
#define __DEM_L1_PHYSICS_LEVEL_H__

#include <Physics/CollisionObjStatic.h>
#include <Data/Array.h>

// Physics level represents a space where physics bodies and collision objects live.

class CAABB;
class btDiscreteDynamicsWorld;

namespace Physics
{

class CPhysicsLevel: public Core::CObject
{
	__DeclareClassNoFactory;

protected:

	btDiscreteDynamicsWorld*	pBtDynWorld;
	float						StepTime;
	CArray<PPhysicsObj>			Objects;

	// Cross-dependence of collision objects and the level.
	// Only objects know, how to add them, but only the level knows, when it is killed.
	// On level term all objects must be removed from it.
	friend class CPhysicsObject;

	bool	AddCollisionObject(CPhysicsObject& Obj);
	void	RemoveCollisionObject(CPhysicsObject& Obj);

public:

	CPhysicsLevel(): pBtDynWorld(nullptr), StepTime(1.f / 60.f) {}
	~CPhysicsLevel();

	bool	Init(const CAABB& Bounds);
	void	Term();
	void	Trigger(float FrameTime);
	void	RenderDebug();

	bool	GetClosestRayContact(const vector3& Start, const vector3& End, U16 Group, U16 Mask, vector3* pOutPos = nullptr, PPhysicsObj* pOutObj = nullptr) const;
	UPTR	GetAllRayContacts(const vector3& Start, const vector3& End, U16 Group, U16 Mask) const;

	//int GetAllShapeContacts(PCollisionShape Shape, const CFilterSet& ExcludeSet, CArray<PEntity>& Result);

	void	SetStepTime(float Secs) { n_assert(Secs > 0.f); StepTime = Secs; }
	float	GetStepTime() const { return StepTime; }
	void	SetGravity(const vector3& NewGravity);
	void	GetGravity(vector3& Out) const;

	//void			SetPointOfInterest(const vector3& NewPOI) { POI = NewPOI; }
	//const vector3&	GetPointOfInterest() const { return POI; }

	btDiscreteDynamicsWorld* GetBtWorld() const { return pBtDynWorld; }
};

typedef Ptr<CPhysicsLevel> PPhysicsLevel;

}

#endif
