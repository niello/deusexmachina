#pragma once
#include <Data/RefCounted.h>
#include <Data/DynamicEnum.h>
#include <Math/Vector3.h>
#include <Math/SIMDMath.h>
#include <LinearMath/btScalar.h>

// Physics level represents a space where physics bodies and collision objects live.

class CAABB;
class btDynamicsWorld;
class btDiscreteDynamicsWorld;

namespace Debug
{
	class CDebugDraw;
}

namespace Physics
{
typedef Ptr<class CPhysicsLevel> PPhysicsLevel;
typedef Ptr<class CPhysicsObject> PPhysicsObject;
struct ITickListener;

class CPhysicsLevel : public Data::CRefCounted
{
protected:

	btDiscreteDynamicsWorld* pBtDynWorld = nullptr;
	float                    StepTime = 1.f / 60.f;

	std::set<ITickListener*> _TickListeners; // TODO: weak ptrs?

	static void BeforeTick(btDynamicsWorld* world, btScalar timeStep);
	static void AfterTick(btDynamicsWorld* world, btScalar timeStep);

public:

	Data::CDynamicEnum32     CollisionGroups;

	CPhysicsLevel(const Math::CAABB& Bounds);
	virtual ~CPhysicsLevel() override;

	void  Update(float dt);
	void  RenderDebug(Debug::CDebugDraw& DebugDraw);

	// FIXME CONSISTENCY: contact enumerators work only with CPhysicsObject but GetClosestRayContact detects hit point for any bullet collision objects!
	bool  GetClosestRayContact(const rtm::vector4f& Start, const rtm::vector4f& End, U32 Group, U32 Mask, rtm::vector4f* pOutPos = nullptr, PPhysicsObject* pOutObj = nullptr, CPhysicsObject* pExclude = nullptr) const;
	UPTR  EnumRayContacts(const rtm::vector4f& Start, const rtm::vector4f& End, U32 Group, U32 Mask, std::function<bool(CPhysicsObject&, const rtm::vector4f&)>&& Callback) const;
	UPTR  EnumObjectContacts(const CPhysicsObject& Object, std::function<bool(CPhysicsObject&, const rtm::vector4f&)>&& Callback) const;
	UPTR  EnumSphereContacts(const rtm::vector4f& Position, float Radius, U32 Group, U32 Mask, std::function<bool(CPhysicsObject&, const rtm::vector4f&)>&& Callback) const;
	UPTR  EnumCapsuleYContacts(const rtm::vector4f& Position, float Radius, float CylinderLength, U32 Group, U32 Mask, std::function<bool(CPhysicsObject&, const rtm::vector4f&)>&& Callback) const;

	void  RegisterTickListener(ITickListener* pListener);
	void  UnregisterTickListener(ITickListener* pListener);

	void  SetStepTime(float Secs) { n_assert(Secs > 0.f); StepTime = Secs; }
	float GetStepTime() const { return StepTime; }
	void  SetGravity(const vector3& NewGravity);
	void  GetGravity(vector3& Out) const;

	auto* GetBtWorld() const { return pBtDynWorld; }
};

}
