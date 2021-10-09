#include "ItemUtils.h"
#include <Items/ItemComponent.h>
#include <Items/ItemContainerComponent.h>
#include <Scene/SceneComponent.h>
#include <Game/ECS/Components/RigidBodyComponent.h>

namespace DEM::RPG
{

bool AddItemsIntoContainer(Game::CGameWorld& World, Game::HEntity Container, Game::HEntity ItemStackEntity, bool Merge)
{
	auto pItemStack = World.FindComponent<CItemStackComponent>(ItemStackEntity);
	if (!pItemStack) return false;

	auto pItem = FindItemComponent<const CItemComponent>(World, ItemStackEntity, *pItemStack);
	if (!pItem) return false;

	auto pContainer = World.FindComponent<CItemContainerComponent>(Container);
	if (!pContainer) return false;

	// Fail if this item can't be placed into the container
	// TODO: split stack, fill available container space!
	// bool flag in args to enable this? return actually added count / remaining stack ID?
	CContainerStats Stats;
	CalcContainerStats(World, *pContainer, Stats);
	if (Stats.FreeWeight < pItemStack->Count * pItem->Weight) return false;
	if (Stats.FreeVolume < pItemStack->Count * pItem->Volume) return false;

	// Try to merge new items into existing stack
	if (Merge && !pItemStack->Modified)
	{
		for (auto ItemEntityID : pContainer->Items)
		{
			auto pStack = World.FindComponent<CItemStackComponent>(ItemEntityID);
			if (pStack && pStack->Prototype == pItemStack->Prototype && !pStack->Modified)
			{
				pStack->Count += pItemStack->Count;
				World.DeleteEntity(ItemStackEntity);
				return true;
			}
		}
	}

	// If not merged, transfer a stack into the container

	World.RemoveComponent<Game::CSceneComponent>(ItemStackEntity);
	World.RemoveComponent<Game::CRigidBodyComponent>(ItemStackEntity);

	// TODO: allow inserting into specified index!
	pContainer->Items.push_back(ItemStackEntity);

	return true;
}
//---------------------------------------------------------------------

//!!!check dropping near another item stack or pile (temporary container)! bool flag 'allow merging'?
bool DropItemsToLocation(Game::CGameWorld& World, Game::HEntity ItemStackEntity, const Math::CTransformSRT& Tfm)
{
	auto pItemStack = World.FindComponent<CItemStackComponent>(ItemStackEntity);
	if (!pItemStack) return false;

	const CItemComponent* pItem = FindItemComponent<const CItemComponent>(World, ItemStackEntity, *pItemStack);
	if (!pItem) return false;

	if (pItem->InLocationModelID)
	{
		auto pSceneComponent = World.AddComponent<Game::CSceneComponent>(ItemStackEntity);
		pSceneComponent->RootNode->RemoveFromParent();
		pSceneComponent->AssetID = pItem->InLocationModelID;
		pSceneComponent->SetLocalTransform(Tfm);
	}

	if (pItem->InLocationPhysicsID)
	{
		auto pPhysicsComponent = World.AddComponent<Game::CRigidBodyComponent>(ItemStackEntity);
		pPhysicsComponent->ShapeAssetID = pItem->InLocationPhysicsID;
		pPhysicsComponent->Mass = pItemStack->Count * pItem->Weight;
		// TODO: physics material, collision group & flags
	}

	return true;
}
//---------------------------------------------------------------------

void CalcContainerStats(Game::CGameWorld& World, const CItemContainerComponent& Container, CContainerStats& OutStats)
{
	OutStats.UsedWeight = 0.f;
	OutStats.UsedVolume = 0.f;
	for (auto ItemEntityID : Container.Items)
	{
		auto pStack = World.FindComponent<const CItemStackComponent>(ItemEntityID);
		if (!pStack) continue;

		if (auto pItem = FindItemComponent<const CItemComponent>(World, ItemEntityID, *pStack))
		{
			OutStats.UsedWeight += pStack->Count * pItem->Weight;
			OutStats.UsedVolume += pStack->Count * pItem->Volume;
		}
	}

	OutStats.FreeWeight = (Container.MaxWeight <= 0.f) ? FLT_MAX : (Container.MaxWeight - OutStats.UsedWeight);
	OutStats.FreeVolume = (Container.MaxVolume <= 0.f) ? FLT_MAX : (Container.MaxVolume - OutStats.UsedVolume);
}
//---------------------------------------------------------------------

}
