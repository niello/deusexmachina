#pragma once
#include <Items/ItemStackComponent.h>

// Utilities and algorithms for item manipulation

namespace DEM::RPG
{

template<typename T>
T* FindItemComponent(const Game::CGameWorld& World, Game::HEntity ItemStackEntity)
{
	if (T* pComponent = World.FindComponent<T>(ItemStackEntity)) return pComponent;
	if (CItemStackComponent* pStack = World.FindComponent<CItemStackComponent>(ItemStackEntity))
		return World.FindComponent<T>(pStack->Prototype);
	return nullptr;
}
//---------------------------------------------------------------------

}
