#include "SteerAction.h"
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <AI/Navigation/NavAgentComponent.h>
#include <Math/Vector2.h>
#include <Core/Factory.h>

namespace DEM::AI
{
RTTI_CLASS_IMPL(Steer, Events::CEventNative);
RTTI_CLASS_IMPL(Turn, Events::CEventNative);
FACTORY_CLASS_IMPL(DEM::AI::CSteerAction, 'STCL', CTraversalAction);

static bool DoGenerateAction(CNavAgentComponent& Agent, Game::CActionQueueComponent& Queue, const Navigate& NavAction,
	dtStraightPathContext& Ctx, dtStatus Status, U8 AreaType, dtPolyRef PolyRef, const vector3& Pos, vector3 Dest)
{
	// TODO: switch to DT_STRAIGHTPATH_ALL_CROSSINGS for controlled area, where each poly can have personal action
	int Options = DT_STRAIGHTPATH_AREA_CROSSINGS;

	Agent.IsTraversingLastEdge = dtStatusSucceed(Status);

	vector3 NextDest;
	bool ActionChanged = false;
	while (!Agent.IsTraversingLastEdge)
	{
		// Check if Dest is close enough to be considered reached
		const bool DestReached =
			std::abs(Pos.y - Dest.y) < Agent.Height &&
			vector3::SqDistance2D(Pos, Dest) < Steer::SqLinearTolerance;

		// Get the next edge tracersal action
		Game::HEntity SmartObject;
		auto pNextAction = Agent.Settings->FindAction(Agent, AreaType, PolyRef, &SmartObject);
		ActionChanged = (!pNextAction || &CSteerAction::RTTI != pNextAction->GetRTTI());
		if (ActionChanged)
		{
			// Action changed, we must finish processing. If we are close enough to the next edge start, let the
			// new action start immediately to avoid regenerating already finished Steer action again and again.
			// New edge can't be an offmesh connection because we already checked it in the navigation system.
			if (pNextAction && DestReached)
				return pNextAction->GenerateAction(Agent, SmartObject, Queue, NavAction, Pos);

			break;
		}

		// Get the next path edge end point
		Status = Agent.pNavQuery->findNextStraightPathPoint(Ctx, NextDest.v, nullptr, &AreaType, &PolyRef, Options);
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

		// Elongate path edge
		Dest = NextDest;

		// That was the last edge, its successful traversal will finish the navigate action
		if (dtStatusSucceed(Status)) Agent.IsTraversingLastEdge = true;
	}

	//!!!FIXME: bad smoothing, bad slowdown!

	if (ActionChanged || Agent.IsTraversingLastEdge)
	{
		// no need to calculate distance to action change, it changes right after Dest, additional distance is zero
	}

	// Try to get existing sub-action of required type
	auto pSteer = Queue.GetImmediateSubAction<Steer>(NavAction);

	// Calculate distance from Dest to next action change point. Used for arrival slowdown.
	float AdditionalDistance;

	//!!!target change may also influence this! other criteria too?
	if (!pSteer || pSteer->_Dest != Dest)
	{
		vector3 Prev = Dest;
		vector3 Curr = NextDest;

		//!!!!!!FIXME: calc distance to the next action change, not to the navigation target!
		AdditionalDistance = vector3::Distance(Prev, Curr);
		while (dtStatusInProgress(Status))
		{
			Prev = Curr;
			Status = Agent.pNavQuery->findNextStraightPathPoint(Ctx, Curr.v, nullptr, &AreaType, &PolyRef, 0);
			if (dtStatusFailed(Status)) break;
			AdditionalDistance += vector3::Distance(Prev, Curr);
		}
	}
	else
	{
		AdditionalDistance = pSteer->_AdditionalDistance;
	}

	//???push only if changed? almost all changes are checked here!
	return !!Queue.PushSubActionForParent<Steer>(NavAction, Dest, NextDest, AdditionalDistance);
}
//---------------------------------------------------------------------

bool CSteerAction::GenerateAction(CNavAgentComponent& Agent, Game::HEntity SmartObject, Game::CActionQueueComponent& Queue,
	const Navigate& NavAction, const vector3& Pos)
{
	//???add shortcut method to corridor? Agent.Corridor.initStraightPathSearch(Agent.pNavQuery, Ctx);
	dtStraightPathContext Ctx;
	if (dtStatusFailed(Agent.pNavQuery->initStraightPathSearch(
		Agent.Corridor.getPos(), Agent.Corridor.getTarget(), Agent.Corridor.getPath(), Agent.Corridor.getPathCount(), Ctx)))
		return false;

	// TODO: switch to DT_STRAIGHTPATH_ALL_CROSSINGS for controlled area, where each poly can have personal action
	int Options = DT_STRAIGHTPATH_AREA_CROSSINGS;

	vector3 Dest;
	unsigned char AreaType;
	dtPolyRef PolyRef;
	dtStatus Status = Agent.pNavQuery->findNextStraightPathPoint(Ctx, Dest.v, nullptr, &AreaType, &PolyRef, Options);
	if (dtStatusFailed(Status)) return false;

	return DoGenerateAction(Agent, Queue, NavAction, Ctx, Status, AreaType, PolyRef, Pos, Dest);
}
//---------------------------------------------------------------------

bool CSteerAction::GenerateAction(CNavAgentComponent& Agent, Game::HEntity SmartObject, Game::CActionQueueComponent& Queue,
	const Navigate& NavAction, const vector3& Pos, const vector3& Dest, const vector3& NextDest)
{
	// When steering comes from an offmesh connection, we know that an offmesh start is already reached,
	// and we must steer to the offmesh end. So we can assume that Pos == Dest, and NextDest is our target.
	// Note that offmesh connection is already skipped in the corridor.

	//???add shortcut method to corridor? Agent.Corridor.initStraightPathSearch(Agent.pNavQuery, Ctx);
	dtStraightPathContext Ctx;
	if (dtStatusFailed(Agent.pNavQuery->initStraightPathSearch(
		Agent.Corridor.getPos(), Agent.Corridor.getTarget(), Agent.Corridor.getPath(), Agent.Corridor.getPathCount(), Ctx)))
		return false;

	const dtPolyRef PolyRef = Agent.Corridor.getFirstPoly();
	U8 AreaType = 0;
	Agent.pNavQuery->getAttachedNavMesh()->getPolyArea(PolyRef, &AreaType);

	return DoGenerateAction(Agent, Queue, NavAction, Ctx, DT_IN_PROGRESS, AreaType, PolyRef, Pos, NextDest);
}
//---------------------------------------------------------------------

bool CSteerAction::GenerateRecoveryAction(Game::CActionQueueComponent& Queue,
	const Navigate& NavAction, const vector3& ValidPos)
{
	return !!Queue.PushSubActionForParent<Steer>(NavAction, ValidPos, ValidPos, -0.f);
}
//---------------------------------------------------------------------

}
