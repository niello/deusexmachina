#include "SmartObjectInteraction.h"
#include <Game/Objects/SmartObjectTargetFilter.h>
#include <Game/ECS/Components/ActionQueueComponent.h>

namespace DEM::Game
{

CSmartObjectInteraction::CSmartObjectInteraction(CStrID InteractionID, std::string_view CursorImage)
	: _InteractionID(InteractionID)
{
	_Name = _InteractionID.ToString();
	_CursorImage = CursorImage;

	AddTarget(std::make_unique<CSmartObjectTargetFilter>(_InteractionID));
}
//---------------------------------------------------------------------

//!!!FIXME: need different implementations for selecting an actor! Now only the first selected actor receives a command!
bool CSmartObjectInteraction::Execute(CInteractionContext& Context, bool Enqueue) const
{
	if (Context.Targets.empty() || !Context.Targets[0].Entity || Context.Actors.empty()) return false;

	auto pWorld = Context.Session->FindFeature<CGameWorld>();
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
