#include "SteerAction.h"
#include <Game/ECS/GameWorld.h>
#include <Game/GameSession.h>
#include <AI/Navigation/NavAgentComponent.h>
#include <Math/SIMDMath.h>
#include <Math/Vector2.h>
#include <Core/Factory.h>

namespace DEM::AI
{
FACTORY_CLASS_IMPL(DEM::AI::CSteerAction, 'STRA', CTraversalAction);

bool CSteerAction::GenerateAction(Game::CGameSession& Session, CNavAgentComponent& Agent, Game::HEntity Actor, Game::HEntity Controller, Game::CActionQueueComponent& Queue,
	Game::HAction NavAction, const rtm::vector4f& Pos)
{
	// If not on navmesh, recover to the nearest valid position
	if (Agent.Mode == ENavigationMode::Recovery)
	{
		const rtm::vector4f ClosestPos = rtm::vector_load3(Agent.Corridor.getPos());
		return !!Queue.PushOrUpdateChild<Steer>(NavAction, ClosestPos, ClosestPos, -0.f);
	}

	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return false;

	const auto PosRaw = Math::FromSIMD3(Pos);
	const float* pPos = (Agent.Mode == ENavigationMode::Offmesh) ? PosRaw.v : Agent.Corridor.getPos();

	//???add shortcut method to corridor? Agent.Corridor.initStraightPathSearch(Agent.pNavQuery, Ctx);
	dtStraightPathContext Ctx;
	if (dtStatusFailed(Agent.pNavQuery->initStraightPathSearch(
		pPos, Agent.Corridor.getTarget(), Agent.Corridor.getPath(), Agent.Corridor.getPathCount(), Ctx)))
		return false;

	const int Options = Agent.Settings->IsAreaControllable(Agent.CurrAreaType) ? DT_STRAIGHTPATH_ALL_CROSSINGS : DT_STRAIGHTPATH_AREA_CROSSINGS;

	vector3 DestRaw;
	unsigned char Flags;
	unsigned char AreaType;
	dtPolyRef PolyRef;
	dtStatus Status = Agent.pNavQuery->findNextStraightPathPoint(Ctx, DestRaw.v, &Flags, &AreaType, &PolyRef, Options);
	if (dtStatusFailed(Status)) return false;

	rtm::vector4f Dest = Math::ToSIMD(DestRaw);
	rtm::vector4f NextDest = Dest;
	bool ActionChanged = false;
	bool NeedSlowdown = true;
	bool LastEdge = dtStatusSucceed(Status);
	while (!LastEdge)
	{
		// Check if Dest is close enough to be considered reached
		const rtm::vector4f ToDest = rtm::vector_sub(Pos, Dest);
		const bool DestReached =
			std::abs(rtm::vector_get_y(ToDest)) < Agent.Height &&
			Math::vector_length_squared_xz(ToDest) < Steer::SqLinearTolerance;

		// Get the next edge traversal action
		Game::HEntity Controller;
		auto pNextAction = Agent.Settings->FindAction(*pWorld, Agent, AreaType, PolyRef, &Controller);
		ActionChanged = (!pNextAction || &CSteerAction::RTTI != pNextAction->GetRTTI());
		if (ActionChanged)
		{
			// Action changed, we must finish processing. If we are close enough to the next edge start, let the
			// new action start immediately to avoid regenerating already finished Steer action again and again.
			// Don't trigger offmesh connections. If it was possible, navigation system would do that.
			if (pNextAction && DestReached && !(Flags & DT_STRAIGHTPATH_OFFMESH_CONNECTION))
				return pNextAction->GenerateAction(Session, Agent, Actor, Controller, Queue, NavAction, Pos);

			// Some actions require no arrival slowdown when approaching action change point
			NeedSlowdown = !pNextAction || pNextAction->NeedSlowdownBeforeStart(Agent);
			break;
		}

		// Get the next path edge end point
		const int Options = Agent.Settings->IsAreaControllable(AreaType) ? DT_STRAIGHTPATH_ALL_CROSSINGS : DT_STRAIGHTPATH_AREA_CROSSINGS;
		Status = Agent.pNavQuery->findNextStraightPathPoint(Ctx, DestRaw.v, &Flags, &AreaType, &PolyRef, Options);
		if (dtStatusFailed(Status)) break;

		NextDest = Math::ToSIMD(DestRaw);

		// If next edge changes direction, use its end position for smooth turning and finish
		if (!DestReached)
		{
			constexpr float COS_SMALL_ANGLE_SQ = 0.99999f;
			const rtm::vector4f ToCurr = rtm::vector_set_y(rtm::vector_sub(Dest, Pos), 0.f);
			const rtm::vector4f ToNext = rtm::vector_set_y(rtm::vector_sub(NextDest, Pos), 0.f);
			const float Dot = rtm::vector_dot3(ToNext, ToCurr);
			if (Dot * Dot < rtm::vector_length_squared3(ToNext) * rtm::vector_length_squared3(ToCurr) * COS_SMALL_ANGLE_SQ)
				break;

			//!!!TODO: slowdown if too big turn is expected!
		}

		// Same action and direction, elongate path edge
		Dest = NextDest;
		LastEdge = dtStatusSucceed(Status);
	}

	// Calculate distance from Dest to next action change point. Used for arrival slowdown.
	float AdditionalDistance = NeedSlowdown ? 0.f : -0.f;

	// No need to calculate if action changed at Dest
	if (!ActionChanged && !LastEdge)
	{
		// Try to get existing sub-action of required type
		auto SteerAction = Queue.GetChild(NavAction);
		auto pSteer = SteerAction.As<Steer>();

		// Recalculate only when destination changes
		// TODO: can add other criteria, like final navigation target change
		if (!pSteer || !rtm::vector_all_equal3(pSteer->_Dest, Dest))
		{
			rtm::vector4f Prev = Dest;
			rtm::vector4f Curr = NextDest;
			while (true)
			{
				AdditionalDistance += rtm::vector_distance3(Prev, Curr);

				// Stop if path ends at Curr
				if (!dtStatusInProgress(Status)) break;

				// Stop if action changes at Curr
				Game::HEntity Controller;
				auto pNextAction = Agent.Settings->FindAction(*pWorld, Agent, AreaType, PolyRef, &Controller);
				if (!pNextAction || &CSteerAction::RTTI != pNextAction->GetRTTI()) break;

				// Get end point of the next edge with the same action
				vector3 Point;
				const int Options = Agent.Settings->IsAreaControllable(AreaType) ? DT_STRAIGHTPATH_ALL_CROSSINGS : DT_STRAIGHTPATH_AREA_CROSSINGS;
				Status = Agent.pNavQuery->findNextStraightPathPoint(Ctx, Point.v, nullptr, &AreaType, &PolyRef, Options);
				if (dtStatusFailed(Status)) break;

				Prev = Curr;
				Curr = Math::ToSIMD(Point);
			}
		}
		else
		{
			// Use value cached in the action
			AdditionalDistance = pSteer->_AdditionalDistance;
		}
	}

	// At the last path edge consider desired final facing
	if (LastEdge) NextDest = rtm::vector_add(Dest, NavAction.As<Navigate>()->_FinalFacing);

	// Update existing action or push the new one
	return !!Queue.PushOrUpdateChild<Steer>(NavAction, Dest, NextDest, AdditionalDistance);
}
//---------------------------------------------------------------------

}
