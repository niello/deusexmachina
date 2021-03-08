#pragma once
#include <Game/Interaction/TargetFilter.h>
#include <Game/Interaction/InteractionContext.h>
#include <Game/Objects/SmartObjectComponent.h>
#include <Game/Objects/SmartObject.h>
#include <Game/ECS/GameWorld.h>

// Accepts only smart objects. Optionally requires them to have specific interaction.

namespace DEM::Game
{

class CSmartObjectTargetFilter : public ITargetFilter
{
protected:

	CStrID _InteractionID;

public:

	CSmartObjectTargetFilter(CStrID InteractionID = CStrID::Empty) : _InteractionID(InteractionID) {}

	virtual bool IsTargetValid(const CInteractionContext& Context, U32 Index) const override
	{
		// Check for smart object component
		const auto& Target = (Index == CURRENT_TARGET) ? Context.Target : Context.SelectedTargets[Index];
		if (!Target.Valid) return false;
		auto pWorld = Context.Session->FindFeature<CGameWorld>();
		if (!pWorld) return false;
		auto pSmartComponent = pWorld->FindComponent<CSmartObjectComponent>(Target.Entity);
		if (!pSmartComponent) return false;

		if (!_InteractionID) return true;

		// Optionally check for an interaction
		auto pSmartAsset = pSmartComponent->Asset->GetObject<CSmartObject>();
		return pSmartAsset && pSmartAsset->HasInteraction(_InteractionID);
	}
};

}
