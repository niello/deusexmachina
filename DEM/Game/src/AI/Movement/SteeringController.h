#pragma once
#include <AI/Navigation/TraversalController.h>
#include <Events/EventNative.h>

// Provides steering traversal action, processed by the character controller
// or similar components. It is the most common case, simple movement.

namespace DEM::AI
{

class Steer : public Events::CEventNative
{
	NATIVE_EVENT_DECL;

public:

	vector3 _Dest;
	vector3 _NextDest;
	float   _AdditionalDistance = 0.f; // Set < 0.f to disable arrival slowdown

	explicit Steer(const vector3& Dest, const vector3& NextDest, float AdditionalDistance)
		: _Dest(Dest), _NextDest(NextDest), _AdditionalDistance(AdditionalDistance)
	{}
};

class Turn : public Events::CEventNative
{
	NATIVE_EVENT_DECL;

public:

	vector3 _LookatDirection;

	explicit Turn(const vector3& LookatDirection) : _LookatDirection(LookatDirection) { _LookatDirection.norm(); }
};

class CSteeringController : public CTraversalController
{
	FACTORY_CLASS_DECL;

protected:

	//

public:

	virtual CStrID FindAction(const CNavAgentComponent& Agent, unsigned char AreaType, dtPolyRef Poly, Game::HEntity* pOutSmartObject) override;
	virtual bool   PushSubAction(Game::CActionQueueComponent& Queue, const Events::CEventBase& ParentAction, CStrID Type,
		const vector3& Dest, const vector3& NextDest, float RemainingDistance, Game::HEntity SmartObject) override;
};

}
