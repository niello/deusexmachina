#pragma once
#include <Game/Interaction/TargetFilter.h>
#include <Game/Interaction/InteractionContext.h>
#include <Game/ECS/GameWorld.h>
#include <AI/Navigation/NavAgentComponent.h>

// Accepts only targets passable by any of selected actors
// FIXME: need any or all?

namespace DEM::Game
{

class CPassableTargetFilter : public ITargetFilter
{
public:

	virtual bool IsTargetValid(const CInteractionContext& Context, U32 Index) const override
	{
		const auto& Target = (Index == CURRENT_TARGET) ? Context.Target : Context.SelectedTargets[Index];
		if (!Target.Valid) return false;
		auto pWorld = Context.Session->FindFeature<CGameWorld>();
		if (!pWorld) return false;

		for (auto Actor : Context.SelectedActors)
		{
			auto pAgent = pWorld->FindComponent<DEM::AI::CNavAgentComponent>(Actor);
			if (!pAgent || !pAgent->pNavQuery) continue;

			const float Extents[3] = { 0.f, pAgent->Height, 0.f };
			dtPolyRef PolyRef = 0;
			vector3 Pos;
			pAgent->pNavQuery->findNearestPoly(Context.Target.Point.v, Extents, pAgent->Settings->GetQueryFilter(),
				&PolyRef, Pos.v);

			// FIXME: can return point over the impassable object if it stands on the navmesh!
			if (PolyRef &&
				vector3::SqDistance2D(Pos, Context.Target.Point) <= 0.001f * 0.001f &&
				std::abs(Pos.y - Context.Target.Point.y) < pAgent->Height)
			{
				return true;
			}
		}

		return false;
	}
};

}