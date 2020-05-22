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

bool CSteerAction::GenerateAction(CNavAgentComponent& Agent, Game::HEntity SmartObject, Game::CActionQueueComponent& Queue,
	const Navigate& NavAction, const vector3& Pos)
{
	const float* pCurrPos = Agent.Corridor.getPos();

	//???add shortcut method to corridor? Agent.Corridor.initStraightPathSearch(Agent.pNavQuery, Ctx);
	dtStraightPathContext Ctx;
	if (dtStatusFailed(Agent.pNavQuery->initStraightPathSearch(
		pCurrPos, Agent.Corridor.getTarget(), Agent.Corridor.getPath(), Agent.Corridor.getPathCount(), Ctx)))
		return false;

	// TODO: switch to DT_STRAIGHTPATH_ALL_CROSSINGS for controlled area, where each poly can have personal action
	int Options = DT_STRAIGHTPATH_AREA_CROSSINGS;

	vector3 NextDest;
	unsigned char Flags;
	unsigned char AreaType;
	dtPolyRef PolyRef;
	dtStatus Status = Agent.pNavQuery->findNextStraightPathPoint(Ctx, NextDest.v, &Flags, &AreaType, &PolyRef, Options);
	if (dtStatusFailed(Status)) return false;

	Agent.IsTraversingLastEdge = !dtStatusInProgress(Status);

	// Advance through path points until the traversal action is generated
	vector3 Dest;
	do
	{
		// Elongate path edge
		Dest = NextDest;

		// That was the last edge, finish
		if (!dtStatusInProgress(Status)) break;

		// If close enough to the next path point, assume it already traversed
		if (!(Flags & DT_STRAIGHTPATH_OFFMESH_CONNECTION) &&
			std::abs(Pos.y - Dest.y) < Agent.Height &&
			vector3::SqDistance2D(Pos, Dest) < Steer::SqLinearTolerance)
		{
			Status = Agent.pNavQuery->findNextStraightPathPoint(Ctx, NextDest.v, &Flags, &AreaType, &PolyRef, Options);
			if (dtStatusFailed(Status)) return false;
			continue;
		}

		// Finish if the next path edge requires different traversal action
		// NB: check even offmesh connections, they can use steering action too
		Game::HEntity NextSmartObject;
		auto pNextAction = Agent.Settings->FindAction(Agent, AreaType, PolyRef, &NextSmartObject);
		if (!pNextAction || &RTTI != pNextAction->GetRTTI() || SmartObject != NextSmartObject) break;

		// Get the next path edge end point
		Status = Agent.pNavQuery->findNextStraightPathPoint(Ctx, NextDest.v, &Flags, &AreaType, &PolyRef, Options);
		if (dtStatusFailed(Status)) break;

		// If next edge changes direction, use its end position for smooth turning and finish
		constexpr float COS_SMALL_ANGLE_SQ = 0.99999f;
		const vector2 ToCurr(Dest.x - pCurrPos[0], Dest.z - pCurrPos[2]);
		const vector2 ToNext(NextDest.x - pCurrPos[0], NextDest.z - pCurrPos[2]);
		const float Dot = ToNext.dot(ToCurr);
		if (Dot * Dot < ToNext.SqLength() * ToCurr.SqLength() * COS_SMALL_ANGLE_SQ)
			break;
	}
	while (true);

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
			Status = Agent.pNavQuery->findNextStraightPathPoint(Ctx, Curr.v, &Flags, &AreaType, &PolyRef, 0);
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
	const Navigate& NavAction, const vector3& Pos, const vector3& Dest, const vector3& NextDest)
{
	// The same as above, but edge is already generated, can only try to elongate it
	NOT_IMPLEMENTED;
	return false;
}
//---------------------------------------------------------------------

bool CSteerAction::GenerateRecoveryAction(Game::CActionQueueComponent& Queue,
	const Navigate& NavAction, const vector3& ValidPos)
{
	return !!Queue.PushSubActionForParent<Steer>(NavAction, ValidPos, ValidPos, -0.f);
}
//---------------------------------------------------------------------

}
