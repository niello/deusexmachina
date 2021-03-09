#pragma once
#include <AI/Navigation/TraversalAction.h>
#include <Events/EventNative.h>

// Provides steering traversal action, processed by the character controller
// or similar components. It is the most common case, simple movement.

namespace DEM::AI
{

class Steer : public Events::CEventNative
{
	NATIVE_EVENT_DECL(Steer, Events::CEventNative);

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

	vector3 _Dest;
	vector3 _NextDest;
	float   _AdditionalDistance = 0.f; // Set < 0.f to disable arrival slowdown

	explicit Steer(const vector3& Dest, const vector3& NextDest, float AdditionalDistance)
		: _Dest(Dest), _NextDest(NextDest), _AdditionalDistance(AdditionalDistance)
	{}
};

class Turn : public Events::CEventNative
{
	NATIVE_EVENT_DECL(Turn, Events::CEventNative);

public:

	// Angle offset (in radians) in which agent is considered facing the lookat direction
	// NB: too low tolerance may lead to float precision errors
	static inline constexpr float AngularTolerance = 0.0001f; // Old was 0.005f

	vector3 _LookatDirection;
	float   _Tolerance;

	explicit Turn(const vector3& LookatDirection, float Tolerance = AngularTolerance)
		: _LookatDirection(LookatDirection)
		, _Tolerance(Tolerance)
	{
		_LookatDirection.norm();
	}
};

class CSteerAction : public CTraversalAction
{
	FACTORY_CLASS_DECL;

public:

	// When steering through the offmesh connection, must reach the start point first
	virtual float GetSqTriggerRadius(float AgentRadius, float OffmeshTriggerRadius) const override { return std::max(Steer::SqLinearTolerance, OffmeshTriggerRadius * OffmeshTriggerRadius); }
	virtual bool  GenerateAction(Game::CGameWorld& World, CNavAgentComponent& Agent, Game::HEntity Controller, Game::CActionQueueComponent& Queue, Game::HAction NavAction, const vector3& Pos) override;
};

}
