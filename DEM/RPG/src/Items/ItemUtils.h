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

//!!!FIXME: to config!
constexpr UPTR MAX_QUICK_SLOTS = 10;
constexpr float QUICK_SLOT_VOLUME = 1.f;

enum class EItemStorage
{
	Inventory,
	QuickSlot,
	Equipment,
	Ground,

	None
};

struct CContainerStats
{
	float  UsedWeight = 0.f;
	float  UsedVolume = 0.f;
	float  FreeVolume = 0.f;
	size_t Price = 0;
};

//???vector or set of entities instead?
U32 AddItemsIntoContainer(Game::CGameWorld& World, Game::HEntity Receiver, Game::HEntity StackID, bool Merge = true, bool Split = true);
U32 AddItemsIntoQuickSlots(Game::CGameWorld& World, Game::HEntity Receiver, Game::HEntity StackID, bool Merge = true, bool Split = true);
bool AddItemsToCharacter(Game::CGameWorld& World, Game::HEntity Receiver, Game::HEntity StackID, EItemStorage PreferredStorage, bool AllowGround, bool Merge = true, bool Split = true);
Game::HEntity AddStackIntoCollection(Game::CGameWorld& World, std::vector<Game::HEntity>& Collection, Game::HEntity StackID, bool Merge = true);
void ShrinkItemCollection(std::vector<Game::HEntity>& Collection);
bool DropItemsToLocation(Game::CGameWorld& World, Game::HEntity StackID, const Math::CTransformSRT& Tfm);
void RemoveItemsFromLocation(Game::CGameWorld& World, Game::HEntity StackID);
void RemoveItemsFromContainer(Game::CGameWorld& World, Game::HEntity StackID, Game::HEntity Container);
void CalcContainerStats(Game::CGameWorld& World, const CItemContainerComponent& Container, CContainerStats& OutStats);
bool CanMergeStacks(const CItemStackComponent& SrcStack, const CItemStackComponent* pDestStack);
float GetItemStackVolume(const Game::CGameWorld& World, Game::HEntity StackID);
Game::HEntity TransferItems(Game::CGameWorld& World, U32 Count, Game::HEntity& SrcSlot, Game::HEntity& DestSlot);

Game::HEntity CreateItemsInLocation(CStrID ItemID, U32 Count, CStrID LevelID, const Math::CTransformSRT& Tfm);
Game::HEntity CreateItemsInContainer(CStrID ItemID, U32 Count, Game::HEntity Container, size_t Index);
bool MoveItemsToLocation(Game::HEntity StackID, Game::HEntity PrevContainer, const Math::CTransformSRT& Tfm);
bool MoveItemsToContainer(Game::HEntity StackID, Game::HEntity PrevContainer, Game::HEntity Container, size_t Index);
bool MoveAllItemsToLocation(Game::HEntity PrevContainer, const Math::CTransformSRT& Tfm /*, T Filter*/); //???ret num of failed stacks?
bool MoveAllItemsToContainer(Game::HEntity PrevContainer, Game::HEntity Container /*, T Filter*/); //???ret num of failed stacks?
void MoveItemsInsideContainer(Game::HEntity Container, size_t OldIndex, size_t NewIndex); //???need variant with stack?
void QueryItemsInShape(Game::CGameWorld& World /*collision shape - sphere, capsule etc*/ /*, T Filter*/); //!!!including piles!
bool TryMergeStacks(Game::HEntity StackID1, Game::HEntity StackID2); //bool allow partial?
Game::HEntity SplitStack(Game::HEntity StackID, U32 AmountToSeparate);
void EnumContainedItems(Game::HEntity Container /*, T Filter, T2 Callback*/);
//IsItemValuable, IsItemTrash - not here, game-specific filters? Or use settings from CItemManager?
//FactionHasItem(faction ID, item ID, count)

template<typename T>
inline T* FindItemComponent(const Game::CGameWorld& World, Game::HEntity StackID, const CItemStackComponent& Stack)
{
	if (T* pComponent = World.FindComponent<T>(StackID)) return pComponent;
	return World.FindComponent<T>(Stack.Prototype);
}
//---------------------------------------------------------------------

template<typename T>
inline T* FindItemComponent(const Game::CGameWorld& World, Game::HEntity StackID)
{
	if (T* pComponent = World.FindComponent<T>(StackID)) return pComponent;
	const CItemStackComponent* pStack = World.FindComponent<const CItemStackComponent>(StackID);
	return pStack ? World.FindComponent<T>(pStack->Prototype) : nullptr;
}
//---------------------------------------------------------------------

}
