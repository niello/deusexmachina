#pragma once
#include <Game/Interaction/TargetFilter.h>
#include <Game/Interaction/InteractionContext.h>

// Accepts only targets selectable by player

namespace DEM::Game
{

class CSelectableTargetFilter : public ITargetFilter
{
public:

	virtual bool IsTargetValid(const CInteractionContext& Context, U32 Index) const override
	{
		const auto& Target = (Index == CURRENT_TARGET) ? Context.Target : Context.SelectedTargets[Index];
		//return _World->FindComponent<CSelectableComponent>(Target.Entity)
		return false;
	}
};

}
