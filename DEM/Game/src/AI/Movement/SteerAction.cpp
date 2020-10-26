#include "SteerAction.h"
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <AI/Navigation/NavAgentComponent.h>
#include <Math/Vector2.h>
#include <Core/Factory.h>

namespace DEM::AI
{
FACTORY_CLASS_IMPL(DEM::AI::CSteerAction, 'STRA', CTraversalAction);

static bool DoGenerateAction(Game::CGameWorld& World, CNavAgentComponent& Agent, Game::CActionQueueComponent& Queue, Game::HAction NavAction,
	dtStraightPathContext& Ctx, dtStatus Status, U8 Flags, U8 AreaType, dtPolyRef PolyRef, const vector3& Pos, vector3 Dest)
{
	Agent.IsTraversingLastEdge = dtStatusSucceed(Status);

	vector3 NextDest = Dest;
	bool ActionChanged = false;
	while (!Agent.IsTraversingLastEdge)
	{
		// Check if Dest is close enough to be considered reached
		const bool DestReached =
			std::abs(Pos.y - Dest.y) < Agent.Height &&
			vector3::SqDistance2D(Pos, Dest) < Steer::SqLinearTolerance;

		// Get the next edge traversal action
		Game::HEntity Controller;
		auto pNextAction = Agent.Settings->FindAction(World, Agent, AreaType, PolyRef, &Controller);
		ActionChanged = (!pNextAction || &CSteerAction::RTTI != pNextAction->GetRTTI());
		if (ActionChanged)
		{
			// Action changed, we must finish processing. If we are close enough to the next edge start, let the
			// new action start immediately to avoid regenerating already finished Steer action again and again.
			// Don't trigger offmesh connections. If it was possible, navigation system would do that.
			if (pNextAction && DestReached && !(Flags & DT_STRAIGHTPATH_OFFMESH_CONNECTION))
				return pNextAction->GenerateAction(World, Agent, Controller, Queue, NavAction, Pos);

			break;
		}

		// Get the next path edge end point
		const int Options = Agent.Settings->IsAreaControllable(AreaType) ? DT_STRAIGHTPATH_ALL_CROSSINGS : DT_STRAIGHTPATH_AREA_CROSSINGS;
		Status = Agent.pNavQuery->findNextStraightPathPoint(Ctx, NextDest.v, &Flags, &AreaType, &PolyRef, Options);
		if (dtStatusFailed(Status)) break;

		// If next edge changes direction, use its end position for smooth turning and finish
		if (!DestReached)
		{
			constexpr float COS_SMALL_ANGLE_SQ = 0.99999f;
			const vector2 ToCurr(Dest.x - Pos.x, Dest.z - Pos.z);
			const vector2 ToNext(NextDest.x - Pos.x, NextDest.z - Pos.z);
			const float Dot = ToNext.dot(ToCurr);
			if (Dot * Dot < ToNext.SqLength() * ToCurr.SqLength() * COS_SMALL_ANGLE_SQ)
				break;
		}

		// Same action and direction, elongate path edge
		Dest = NextDest;

		// That was the last edge, its successful traversal will finish the navigate action
		if (dtStatusSucceed(Status)) Agent.IsTraversingLastEdge = true;
	}

	// Calculate distance from Dest to next action change point. Used for arrival slowdown.
	float AdditionalDistance = 0.f;

	// No need to calculate if action changed at Dest
	if (!ActionChanged && !Agent.IsTraversingLastEdge)
	{
		// Try to get existing sub-action of required type
		auto SteerAction = Queue.GetChild(NavAction);
		auto pSteer = SteerAction.As<Steer>();

		// Recalculate only when destination changes
		// TODO: can add other criteria, like final navigation target change
		if (!pSteer || pSteer->_Dest != Dest)
		{
			vector3 Prev = Dest;
			vector3 Curr = NextDest;
			while (true)
			{
				AdditionalDistance += vector3::Distance(Prev, Curr);

				// Stop if path ends at Curr
				if (!dtStatusInProgress(Status)) break;

				// Stop if action changes at Curr
				Game::HEntity Controller;
				auto pNextAction = Agent.Settings->FindAction(World, Agent, AreaType, PolyRef, &Controller);
				if (!pNextAction || &CSteerAction::RTTI != pNextAction->GetRTTI()) break;

				// Get end point of the next edge with the same action
				Prev = Curr;
				const int Options = Agent.Settings->IsAreaControllable(AreaType) ? DT_STRAIGHTPATH_ALL_CROSSINGS : DT_STRAIGHTPATH_AREA_CROSSINGS;
				Status = Agent.pNavQuery->findNextStraightPathPoint(Ctx, Curr.v, nullptr, &AreaType, &PolyRef, Options);
				if (dtStatusFailed(Status)) break;
			}
		}
		else
		{
			// Use value cached in the action
			AdditionalDistance = pSteer->_AdditionalDistance;
		}
	}

	// Update existing action or push the new one
	return !!Queue.PushOrUpdateChild<Steer>(NavAction, Dest, NextDest, AdditionalDistance);
}
//---------------------------------------------------------------------

bool CSteerAction::GenerateAction(Game::CGameWorld& World, CNavAgentComponent& Agent, Game::HEntity Controller, Game::CActionQueueComponent& Queue,
	Game::HAction NavAction, const vector3& Pos)
{
	//???add shortcut method to corridor? Agent.Corridor.initStraightPathSearch(Agent.pNavQuery, Ctx);
	dtStraightPathContext Ctx;
	if (dtStatusFailed(Agent.pNavQuery->initStraightPathSearch(
		Agent.Corridor.getPos(), Agent.Corridor.getTarget(), Agent.Corridor.getPath(), Agent.Corridor.getPathCount(), Ctx)))
		return false;

	const int Options = Agent.Settings->IsAreaControllable(Agent.CurrAreaType) ? DT_STRAIGHTPATH_ALL_CROSSINGS : DT_STRAIGHTPATH_AREA_CROSSINGS;

	vector3 Dest;
	unsigned char Flags;
	unsigned char AreaType;
	dtPolyRef PolyRef;
	dtStatus Status = Agent.pNavQuery->findNextStraightPathPoint(Ctx, Dest.v, &Flags, &AreaType, &PolyRef, Options);
	if (dtStatusFailed(Status)) return false;

	return DoGenerateAction(World, Agent, Queue, NavAction, Ctx, Status, Flags, AreaType, PolyRef, Pos, Dest);
}
//---------------------------------------------------------------------

//???need both GenerateAction variants really? can unify? pointers for Dest & NextDest?
bool CSteerAction::GenerateAction(Game::CGameWorld& World, CNavAgentComponent& Agent, Game::HEntity Controller, Game::CActionQueueComponent& Queue,
	Game::HAction NavAction, const vector3& Pos, const vector3& Dest, const vector3& NextDest)
{
	// When steering comes from an offmesh connection, we know that an offmesh start is already reached,
	// and we must steer to the offmesh end. So we can assume that Pos == Dest, and NextDest is our target.
	// Note that offmesh connection is already skipped in the corridor.

	//???add shortcut method to corridor? Agent.Corridor.initStraightPathSearch(Agent.pNavQuery, Ctx);
	dtStraightPathContext Ctx;
	if (dtStatusFailed(Agent.pNavQuery->initStraightPathSearch(
		Agent.Corridor.getPos(), Agent.Corridor.getTarget(), Agent.Corridor.getPath(), Agent.Corridor.getPathCount(), Ctx)))
		return false;

	// FIXME: ensure that Agent.CurrAreaType is always actual here!

	return DoGenerateAction(World, Agent, Queue, NavAction, Ctx,
		DT_IN_PROGRESS, 0, Agent.CurrAreaType, Agent.Corridor.getFirstPoly(), Pos, NextDest);
}
//---------------------------------------------------------------------

bool CSteerAction::GenerateRecoveryAction(Game::CActionQueueComponent& Queue,
	Game::HAction NavAction, const vector3& ValidPos)
{
	return !!Queue.PushOrUpdateChild<Steer>(NavAction, ValidPos, ValidPos, -0.f);
}
//---------------------------------------------------------------------

}
