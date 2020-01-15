#pragma once
#include <Data/RefCounted.h>
#include <Data/DynamicEnum.h>
#include <Math/Vector3.h>

// Physics level represents a space where physics bodies and collision objects live.

class CAABB;
class btDiscreteDynamicsWorld;

namespace Debug
{
	class CDebugDraw;
}

namespace Physics
{
typedef Ptr<class CPhysicsLevel> PPhysicsLevel;
//typedef Ptr<class CCollisionShape> PCollisionShape;
typedef Ptr<class CPhysicsObject> PPhysicsObject;

class CPhysicsLevel : public Data::CRefCounted
{
protected:

	btDiscreteDynamicsWorld* pBtDynWorld = nullptr;
	float                    StepTime = 1.f / 60.f;

public:

	Data::CDynamicEnum16     CollisionGroups;

	CPhysicsLevel(const CAABB& Bounds);
	virtual ~CPhysicsLevel() override;

	void  Update(float dt);

	void  SetDebugRenderer(Debug::CDebugDraw* pDebugDraw);
	void  RenderDebug();

	bool  GetClosestRayContact(const vector3& Start, const vector3& End, U16 Group, U16 Mask, vector3* pOutPos = nullptr, PPhysicsObject* pOutObj = nullptr) const;
	UPTR  GetAllRayContacts(const vector3& Start, const vector3& End, U16 Group, U16 Mask) const;

	//int GetAllShapeContacts(PCollisionShape Shape, const CFilterSet& ExcludeSet, CArray<PEntity>& Result);

	void  SetStepTime(float Secs) { n_assert(Secs > 0.f); StepTime = Secs; }
	float GetStepTime() const { return StepTime; }
	void  SetGravity(const vector3& NewGravity);
	void  GetGravity(vector3& Out) const;

	auto* GetBtWorld() const { return pBtDynWorld; }
};

}
