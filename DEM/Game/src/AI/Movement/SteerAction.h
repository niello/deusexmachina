#pragma once
#include <AI/Navigation/TraversalAction.h>
#include <Events/EventNative.h>

// Provides steering traversal action, processed by the character controller
// or similar components. It is the most common case, simple movement.

namespace DEM::AI
{

class Steer : public Events::CEventNative
{
	NATIVE_EVENT_DECL;

public:

	// Squared range (in meters) in which agent is considered being at the destination
	// NB: too low tolerance may lead to float precision errors
	static inline constexpr float SqLinearTolerance = 0.0004f * 0.0004f;

	// TODO: check this old info, may be still actual, then must adjust current tolerances
	//// At very small speeds and physics step sizes body position stops updating because of limited
	//// float precision. LinearSpeed = 0.0007f, StepSize = 0.01f, Pos + LinearSpeed * StepSize = Pos.
	//// So body never reaches the desired destination and we must accept arrival at given tolerance.
	//// The less is this value, the more precise is resulting position, but the more time arrival takes.
	//// Empirical minimum value is somewhere around 0.0008f.
	//// This value is measured in game world meters.
	//const float LinearArrivalTolerance = 0.009f;

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

	// Angle offset (in radians) in which agent is considered facing the lookat direction
	// NB: too low tolerance may lead to float precision errors
	static inline constexpr float AngularTolerance = 0.0001f; // Old was 0.005f

	vector3 _LookatDirection;

	explicit Turn(const vector3& LookatDirection) : _LookatDirection(LookatDirection) { _LookatDirection.norm(); }
};

class CSteerAction : public CTraversalAction
{
	FACTORY_CLASS_DECL;

protected:

	//

public:

	virtual bool   CanSkipPathPoint(float SqDistance) const override { return SqDistance < Steer::SqLinearTolerance; }
	virtual U8     PushSubAction(Game::CActionQueueComponent& Queue, const Events::CEventBase& ParentAction,
		const vector3& Dest, const vector3& NextDest, Game::HEntity SmartObject) override;
	virtual void   SetDistanceToTarget(Game::CActionQueueComponent& Queue, const Events::CEventBase& ParentAction, float Distance) override;
};

}
