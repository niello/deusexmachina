#include "SmartObjectInteraction.h"
#include <Game/Interaction/InteractionContext.h>
#include <Game/Objects/SmartObjectComponent.h>
#include <Game/Objects/SmartObject.h>
#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>

namespace DEM::Game
{

CSmartObjectInteraction::CSmartObjectInteraction(CStrID InteractionID, std::string_view CursorImage)
	: _InteractionID(InteractionID)
{
	_Name = _InteractionID.ToString();
	_CursorImage = CursorImage;
}
//---------------------------------------------------------------------

bool IsTargetSmartObject(const CGameSession& Session, const CInteractionContext& Context, U32 Index, CStrID InteractionID)
{
	// Check for smart object component
	const auto& Target = (Index == Context.SelectedTargetCount) ? Context.CandidateTarget : Context.Targets[Index];
	if (!Target.Valid) return false;
	auto pWorld = Session.FindFeature<CGameWorld>();
	if (!pWorld) return false;
	auto pSmartComponent = pWorld->FindComponent<CSmartObjectComponent>(Target.Entity);
	if (!pSmartComponent) return false;

	if (!InteractionID) return true;

	// Optionally check for an interaction
	auto pSmartAsset = pSmartComponent->Asset->GetObject<CSmartObject>();
	return pSmartAsset && pSmartAsset->HasInteraction(InteractionID);
}
//---------------------------------------------------------------------

bool CSmartObjectInteraction::IsTargetValid(const CGameSession& Session, U32 Index, const CInteractionContext& Context) const
{
	// FIXME: CStrID conversion
	return Index == 0 && IsTargetSmartObject(Session, Context, Index, CStrID(_Name.c_str()));
}
//---------------------------------------------------------------------

//!!!FIXME: need different implementations for selecting an actor! Now only the first selected actor receives a command!
bool CSmartObjectInteraction::Execute(CGameSession& Session, CInteractionContext& Context, bool Enqueue) const
{
	if (Context.Targets.empty() || !Context.Targets[0].Entity || Context.Actors.empty()) return false;

	auto pWorld = Session.FindFeature<CGameWorld>();
	if (!pWorld) return false;
	auto pSOComponent = pWorld->FindComponent<CSmartObjectComponent>(Context.Targets[0].Entity);
	if (!pSOComponent || !pSOComponent->Asset) return false;
	CSmartObject* pSOAsset = pSOComponent->Asset->GetObject<CSmartObject>();
	if (!pSOAsset) return false;

	auto pQueue = pWorld->FindComponent<CActionQueueComponent>(Context.Actors[0]);
	if (!pQueue) return false;

	if (!Enqueue) pQueue->Reset();
	pQueue->EnqueueAction<InteractWithSmartObject>(_InteractionID, Context.Targets[0].Entity);

	return true;
}
//---------------------------------------------------------------------

}
