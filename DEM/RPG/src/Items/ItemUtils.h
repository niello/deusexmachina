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
struct CItemContainerComponent;

struct CContainerStats
{
	float UsedWeight = 0.f;
	float UsedVolume = 0.f;
	float FreeWeight = 0.f;
	float FreeVolume = 0.f;
};

//???vector or set of entities instead?
bool AddItemsIntoContainer(Game::CGameWorld& World, Game::HEntity Container, Game::HEntity ItemStackEntity, bool Merge = true);
bool DropItemsToLocation(Game::CGameWorld& World, Game::HEntity ItemStackEntity, const Math::CTransformSRT& Tfm);
void CalcContainerStats(Game::CGameWorld& World, const CItemContainerComponent& Container, CContainerStats& OutStats);

template<typename T>
T* FindItemComponent(const Game::CGameWorld& World, Game::HEntity ItemStackEntity, const CItemStackComponent& Stack)
{
	if (T* pComponent = World.FindComponent<T>(ItemStackEntity)) return pComponent;
	return World.FindComponent<T>(Stack.Prototype);
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
