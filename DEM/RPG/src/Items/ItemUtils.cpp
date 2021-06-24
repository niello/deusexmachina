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

	// Respect container limitations
	if (auto pContainer = World.FindComponent<const CItemContainerComponent>(Container))
	{
		if (pContainer->MaxWeight > 0.f || pContainer->MaxVolume > 0.f)
		{
			// Collect weight and volume already occupied in a container
			float Weight = 0.f;
			float Volume = 0.f;
			World.ForEachComponent<const CItemStackComponent>(
				[&Weight, &Volume, &World, Container](auto EntityID, const CItemStackComponent& Stack)
			{
				if (Stack.Container != Container) return;

				if (const CItemComponent* pItem = FindItemComponent<const CItemComponent>(World, EntityID, Stack))
				{
					Weight += Stack.Count * pItem->Weight;
					Volume += Stack.Count * pItem->Volume;
				}
			});

			// Fail if this item can't be placed into the container
			// TODO: split stack, fll available container space!
			const float NewItemWeight = pItemStack->Count * pItem->Weight;
			if (pContainer->MaxWeight > 0.f && Weight + NewItemWeight > pContainer->MaxWeight) return false;
			const float NewItemVolume = pItemStack->Count * pItem->Volume;
			if (pContainer->MaxVolume > 0.f && Volume + NewItemVolume > pContainer->MaxVolume) return false;
		}
	}

	// Try to merge new items into existing stack
	if (Merge && !pItemStack->Modified)
	{
		CItemStackComponent* pMergeInto = nullptr;
		World.ForEachComponent<CItemStackComponent>(
			[&pMergeInto, &World, Container, pItemStack](auto EntityID, CItemStackComponent& Stack)
		{
			//!!!FIXME: need to break loop once found!
			if (Stack.Container == Container && Stack.Prototype == pItemStack->Prototype && !Stack.Modified)
				pMergeInto = &Stack;
		});

		if (pMergeInto)
		{
			pMergeInto->Count += pItemStack->Count;
			World.DeleteEntity(ItemStackEntity);
			return true;
		}
	}

	// If not merged, simply transfer a stack into the container

	World.RemoveComponent<Game::CSceneComponent>(ItemStackEntity);
	World.RemoveComponent<Game::CRigidBodyComponent>(ItemStackEntity);

	pItemStack->Container = Container;

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

	pItemStack->Container = {};

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

}
