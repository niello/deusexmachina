#pragma once
#include <Game/Interaction/TargetFilter.h>
#include <Game/Interaction/InteractionContext.h>
#include <Game/Interaction/SelectableComponent.h>
#include <Game/ECS/GameWorld.h>

// Accepts only targets selectable by player

namespace DEM::Game
{

class CSelectableTargetFilter : public ITargetFilter
{
public:

	virtual bool IsTargetValid(const CInteractionContext& Context, U32 Index) const override
	{
		const auto& Target = (Index == CURRENT_TARGET) ? Context.CandidateTarget : Context.Targets[Index];
		if (!Target.Valid) return false;
		auto pWorld = Context.Session->FindFeature<CGameWorld>();
		return pWorld && pWorld->FindComponent<CSelectableComponent>(Target.Entity);
	}
};

}
