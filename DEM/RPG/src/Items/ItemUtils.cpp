#include "ItemUtils.h"
#include <Items/ItemComponent.h>
#include <Scene/SceneComponent.h>
#include <Game/ECS/Components/RigidBodyComponent.h>

namespace DEM::RPG
{

bool AddItemsIntoContainer(Game::CGameWorld& World, Game::HEntity Container, Game::HEntity ItemStackEntity)
{
	auto pItemStack = World.FindComponent<CItemStackComponent>(ItemStackEntity);
	if (!pItemStack) return false;

	//!!!only if container entity has a container component with limits!
	// Collect weight and volume already occupied in a container
	float Weight = 0.f;
	float Volume = 0.f;
	World.ForEachComponent<const CItemStackComponent>(
		[&Weight, &Volume, &World, Container](auto EntityID, const CItemStackComponent& Stack)
	{
		if (Stack.Container != Container) return;

		if (const CItemComponent* pItem = FindItemComponent<const CItemComponent>(World, EntityID, &Stack))
		{
			Weight += Stack.Count * pItem->Weight;
			Volume += Stack.Count * pItem->Volume;
		}
	});

	if (const CItemComponent* pItem = FindItemComponent<const CItemComponent>(World, ItemStackEntity, pItemStack))
	{
		//NewItemWeight = pItemStack->Count * pItem->Weight;
		//NewItemVolume = pItemStack->Count * pItem->Volume;

		// if Weight + NewItemWeight > ContainerWeightLimit, fail
		// if Volume + NewItemVolume > ContainerVolumeLimit, fail
	}

	World.RemoveComponent<Game::CSceneComponent>(ItemStackEntity);
	World.RemoveComponent<Game::CRigidBodyComponent>(ItemStackEntity);

	pItemStack->Container = Container;

	return true;
}
//---------------------------------------------------------------------

//!!!check dropping near another item stack or pile! bool flag 'allow merging'?
bool DropItemsToLocation(Game::CGameWorld& World, Game::HEntity ItemStackEntity, const Math::CTransformSRT& Tfm)
{
	auto pItemStack = World.FindComponent<CItemStackComponent>(ItemStackEntity);
	if (!pItemStack) return false;

	const CItemComponent* pItem = FindItemComponent<const CItemComponent>(World, ItemStackEntity, pItemStack);
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

}
