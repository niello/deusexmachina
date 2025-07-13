#pragma once
#include <AI/Navigation/TraversalAction.h>
#include <AI/Command.h>
#include <rtm/vector4f.h>

// Provides steering traversal action, processed by the character controller
// or similar components. It is the most common case, simple movement.

namespace DEM::AI
{

class Steer : public CCommand
{
	RTTI_CLASS_DECL(DEM::AI::Steer, CCommand);

public:

	// Squared range (in meters) in which agent is considered being at the destination
	// NB: too low tolerance may lead to float precision errors
	static constexpr float LinearTolerance = 0.0004f;
	static constexpr float SqLinearTolerance = LinearTolerance * LinearTolerance;

	rtm::vector4f _Dest;
	rtm::vector4f _NextDest;
	float         _AdditionalDistance; // Set any negative (std::signbit check) to disable arrival slowdown

	void SetPayload(rtm::vector4f_arg0 Dest, rtm::vector4f_arg1 NextDest, float AdditionalDistance)
	{
		_Dest = Dest;
		_NextDest = NextDest;
		_AdditionalDistance = AdditionalDistance;
	}
};

class Turn : public CCommand
{
	RTTI_CLASS_DECL(DEM::AI::Turn, CCommand);

public:

	// Angle offset (in radians) in which agent is considered facing the lookat direction
	// NB: too low tolerance may lead to float precision errors
	static constexpr float AngularTolerance = 0.0001f; // Old was 0.005f

	rtm::vector4f _LookatDirection;
	float         _Tolerance;

	void SetPayload(rtm::vector4f_arg0 LookatDirection, float Tolerance = AngularTolerance)
	{
		_LookatDirection = rtm::vector_normalize3(LookatDirection);
		_Tolerance = Tolerance;
	}
};

class CSteerAction : public CTraversalAction
{
	FACTORY_CLASS_DECL;

public:

	// When steering through the offmesh connection, must reach the start point first
	virtual float GetSqTriggerRadius(float AgentRadius, float OffmeshTriggerRadius) const override { return std::max(Steer::SqLinearTolerance, OffmeshTriggerRadius * OffmeshTriggerRadius); }
	virtual bool  GenerateAction(Game::CGameSession& Session, CNavAgentComponent& Agent, Game::HEntity Actor, Game::HEntity Controller, const rtm::vector4f& Pos, CCommandStackComponent& CmdStack, Navigate& NavigateCmd) override;
};

}
