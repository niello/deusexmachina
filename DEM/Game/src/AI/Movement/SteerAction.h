#pragma once
#include <AI/Navigation/TraversalAction.h>
#include <Events/EventNative.h>
#include <rtm/vector4f.h>

// Provides steering traversal action, processed by the character controller
// or similar components. It is the most common case, simple movement.

namespace DEM::AI
{

class Steer : public ::Events::CEventNative
{
	NATIVE_EVENT_DECL(Steer, ::Events::CEventNative);

public:

	// Squared range (in meters) in which agent is considered being at the destination
	// NB: too low tolerance may lead to float precision errors
	static inline constexpr float LinearTolerance = 0.0004f;
	static inline constexpr float SqLinearTolerance = LinearTolerance * LinearTolerance;

	// TODO: check this old info, may be still actual, then must adjust current tolerances
	//// At very small speeds and physics step sizes body position stops updating because of limited
	//// float precision. LinearSpeed = 0.0007f, StepSize = 0.01f, Pos + LinearSpeed * StepSize = Pos.
	//// So body never reaches the desired destination and we must accept arrival at given tolerance.
	//// The less is this value, the more precise is resulting position, but the more time arrival takes.
	//// Empirical minimum value is somewhere around 0.0008f.
	//// This value is measured in game world meters.
	//const float LinearArrivalTolerance = 0.009f;

	rtm::vector4f _Dest;
	rtm::vector4f _NextDest;
	float         _AdditionalDistance = 0.f; // Set < 0.f to disable arrival slowdown

	explicit Steer(const rtm::vector4f& Dest, const rtm::vector4f& NextDest, float AdditionalDistance)
		: _Dest(Dest), _NextDest(NextDest), _AdditionalDistance(AdditionalDistance)
	{}
};

class Turn : public ::Events::CEventNative
{
	NATIVE_EVENT_DECL(Turn, ::Events::CEventNative);

public:

	// Angle offset (in radians) in which agent is considered facing the lookat direction
	// NB: too low tolerance may lead to float precision errors
	static inline constexpr float AngularTolerance = 0.0001f; // Old was 0.005f

	rtm::vector4f _LookatDirection;
	float         _Tolerance;

	explicit Turn(const rtm::vector4f& LookatDirection, float Tolerance = AngularTolerance)
		: _LookatDirection(rtm::vector_normalize3(LookatDirection))
		, _Tolerance(Tolerance)
	{
	}
};

class CSteerAction : public CTraversalAction
{
	FACTORY_CLASS_DECL;

public:

	// When steering through the offmesh connection, must reach the start point first
	virtual float GetSqTriggerRadius(float AgentRadius, float OffmeshTriggerRadius) const override { return std::max(Steer::SqLinearTolerance, OffmeshTriggerRadius * OffmeshTriggerRadius); }
	virtual bool  GenerateAction(Game::CGameSession& Session, CNavAgentComponent& Agent, Game::HEntity Actor, Game::HEntity Controller, Game::CActionQueueComponent& Queue, Game::HAction NavAction, const vector3& Pos) override;
};

}
