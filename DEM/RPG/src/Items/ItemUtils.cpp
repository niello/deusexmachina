#include "ItemUtils.h"
#include <Items/ItemComponent.h>
#include <Items/ItemContainerComponent.h>
#include <Scene/SceneComponent.h>
#include <Physics/RigidBodyComponent.h>

namespace DEM::RPG
{

Game::HEntity AddItemsIntoContainer(Game::CGameWorld& World, Game::HEntity Container, Game::HEntity StackID, bool Merge)
{
	auto pItemStack = World.FindComponent<CItemStackComponent>(StackID);
	if (!pItemStack) return {};

	auto pItem = FindItemComponent<const CItemComponent>(World, StackID, *pItemStack);
	if (!pItem) return {};

	auto pContainer = World.FindComponent<CItemContainerComponent>(Container);
	if (!pContainer) return {};

	// Check that we don't insert already contained stack, and find a merge stack if posible
	if (std::find(pContainer->Items.cbegin(), pContainer->Items.cend(), StackID) != pContainer->Items.cend())
		return {};

	// Fail if this item can't be placed into the container
	// TODO: split stack, fill available container space!
	// bool flag in args to enable this? return actually added count / remaining stack ID?
	CContainerStats Stats;
	CalcContainerStats(World, *pContainer, Stats);
	if (Stats.FreeWeight < pItemStack->Count * pItem->Weight) return {};
	if (Stats.FreeVolume < pItemStack->Count * pItem->Volume) return {};

	// Try to merge new items into existing stack
	if (Merge && !pItemStack->Modified)
	{
		for (auto MergeAcceptorID : pContainer->Items)
		{
			auto pMergeTo = World.FindComponent<CItemStackComponent>(MergeAcceptorID);
			if (pMergeTo && pMergeTo->Prototype == pItemStack->Prototype && !pMergeTo->Modified)
			{
				pMergeTo->Count += pItemStack->Count;
				World.DeleteEntity(StackID);
				return MergeAcceptorID;
			}
		}
	}

	// If not merged, transfer a stack into the container
	RemoveItemsFromLocation(World, StackID);

	// TODO: allow inserting into specified index!
	pContainer->Items.push_back(StackID);

	return StackID;
}
//---------------------------------------------------------------------

//!!!check dropping near another item stack or pile (temporary container)! bool flag 'allow merging'?
bool DropItemsToLocation(Game::CGameWorld& World, Game::HEntity ItemStackEntity, const Math::CTransformSRT& Tfm)
{
	auto pItemStack = World.FindComponent<CItemStackComponent>(ItemStackEntity);
	if (!pItemStack) return false;

	const CItemComponent* pItem = FindItemComponent<const CItemComponent>(World, ItemStackEntity, *pItemStack);
	if (!pItem) return false;

	// TODO:
	// If merging enabled, query item stacks and tmp item containers in accessible range, not owned by someone else
	// If tmp item containers are found, add the stack to the closest one (take lookat dir into account?)
	// Else if stacks are found, try to merge into the closest one (take lookat dir into account?)
	// If not merged, create tmp item container and add found stack and our stack to it
	// If nothing found, drop a new item object into location (create scene and RB)

	if (pItem->WorldModelID)
	{
		auto pSceneComponent = World.AddComponent<Game::CSceneComponent>(ItemStackEntity);
		pSceneComponent->RootNode->RemoveFromParent();
		pSceneComponent->AssetID = pItem->WorldModelID;
		pSceneComponent->SetLocalTransform(Tfm);
	}

	if (pItem->WorldPhysicsID)
	{
		auto pPhysicsComponent = World.AddComponent<Game::CRigidBodyComponent>(ItemStackEntity);
		pPhysicsComponent->ShapeAssetID = pItem->WorldPhysicsID;
		pPhysicsComponent->Mass = pItemStack->Count * pItem->Weight;
		pPhysicsComponent->CollisionGroupID = CStrID("PhysicalDynamic|Interactable");
		// TODO: physics material?
	}

	return true;
}
//---------------------------------------------------------------------

void RemoveItemsFromLocation(Game::CGameWorld& World, Game::HEntity StackID)
{
	World.RemoveComponent<Game::CSceneComponent>(StackID);
	World.RemoveComponent<Game::CRigidBodyComponent>(StackID);
}
//---------------------------------------------------------------------

// TODO: use container as an std::optional hint? or handle this in user API and call RemoveItemsFromContainer only when needed?
void RemoveItemsFromContainer(Game::CGameWorld& World, Game::HEntity ItemStackEntity, Game::HEntity Container)
{
	auto pContainer = World.FindComponent<CItemContainerComponent>(Container);
	if (!pContainer) return;

	pContainer->Items.erase(std::remove(pContainer->Items.begin(), pContainer->Items.end(), ItemStackEntity));
}
//---------------------------------------------------------------------

void CalcContainerStats(Game::CGameWorld& World, const CItemContainerComponent& Container, CContainerStats& OutStats)
{
	OutStats.UsedWeight = 0.f;
	OutStats.UsedVolume = 0.f;
	OutStats.Price = 0;
	for (auto ItemEntityID : Container.Items)
	{
		auto pStack = World.FindComponent<const CItemStackComponent>(ItemEntityID);
		if (!pStack) continue;

		if (auto pItem = FindItemComponent<const CItemComponent>(World, ItemEntityID, *pStack))
		{
			OutStats.UsedWeight += pStack->Count * pItem->Weight;
			OutStats.UsedVolume += pStack->Count * pItem->Volume;
			OutStats.Price += pStack->Count * pItem->Price; //???what to count? only valuable or money-like items?
		}
	}

	OutStats.FreeWeight = (Container.MaxWeight <= 0.f) ? FLT_MAX : (Container.MaxWeight - OutStats.UsedWeight);
	OutStats.FreeVolume = (Container.MaxVolume <= 0.f) ? FLT_MAX : (Container.MaxVolume - OutStats.UsedVolume);
}
//---------------------------------------------------------------------

}
