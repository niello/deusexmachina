#pragma once
#include <Data/Ptr.h>
#include <Data/Array.h>
#include <Math/Vector3.h>

// Physics level represents a space where physics bodies and collision objects live.

class CAABB;
class btDiscreteDynamicsWorld;

namespace Physics
{
typedef Ptr<class CPhysicsObject> PPhysicsObject;
typedef std::unique_ptr<class CPhysicsLevel> PPhysicsLevel;

class CPhysicsLevel final
{
protected:

	btDiscreteDynamicsWorld* pBtDynWorld = nullptr;
	float                    StepTime = 1.f / 60.f;

public:

	CPhysicsLevel(const CAABB& Bounds);
	~CPhysicsLevel();

	void	Update(float dt);
	void	RenderDebug();

	bool	GetClosestRayContact(const vector3& Start, const vector3& End, U16 Group, U16 Mask, vector3* pOutPos = nullptr, PPhysicsObject* pOutObj = nullptr) const;
	UPTR	GetAllRayContacts(const vector3& Start, const vector3& End, U16 Group, U16 Mask) const;

	//int GetAllShapeContacts(PCollisionShape Shape, const CFilterSet& ExcludeSet, CArray<PEntity>& Result);

	void	SetStepTime(float Secs) { n_assert(Secs > 0.f); StepTime = Secs; }
	float	GetStepTime() const { return StepTime; }
	void	SetGravity(const vector3& NewGravity);
	void	GetGravity(vector3& Out) const;

	btDiscreteDynamicsWorld* GetBtWorld() const { return pBtDynWorld; }
};

}
