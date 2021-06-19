#pragma once
#include <Items/ItemStackComponent.h>
#include <Game/ECS/GameWorld.h>

// Utilities and algorithms for item manipulation

namespace Math
{
	class CTransformSRT;
}

namespace DEM::RPG
{

//???vector or set of entities instead?
bool AddItemsIntoContainer(Game::CGameWorld& World, Game::HEntity Container, Game::HEntity ItemStackEntity);
bool DropItemsToLocation(Game::CGameWorld& World, Game::HEntity ItemStackEntity, const Math::CTransformSRT& Tfm);

template<typename T>
T* FindItemComponent(const Game::CGameWorld& World, Game::HEntity ItemStackEntity, const CItemStackComponent* pStack)
{
	if (T* pComponent = World.FindComponent<T>(ItemStackEntity)) return pComponent;
	return pStack ? World.FindComponent<T>(pStack->Prototype) : nullptr;
}
//---------------------------------------------------------------------

template<typename T>
T* FindItemComponent(const Game::CGameWorld& World, Game::HEntity ItemStackEntity)
{
	if (T* pComponent = World.FindComponent<T>(ItemStackEntity)) return pComponent;
	if (const CItemStackComponent* pStack = World.FindComponent<const CItemStackComponent>(ItemStackEntity))
		return World.FindComponent<T>(pStack->Prototype);
	return nullptr;
}
//---------------------------------------------------------------------

}
