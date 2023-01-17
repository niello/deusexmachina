#pragma once
#include <Items/ItemStackComponent.h>
#include <Items/EquipmentComponent.h>
#include <Game/ECS/GameWorld.h>

// Utilities and algorithms for item manipulation
// NB: storage manipulation functions maintain data integrity inside the single storage, but not across them.
// I.e. if the item is moved inside one container or equipment, it will never be lost or duplicated. If the item
// is moved from one container (or other storage) to another then the calling code is obliged to maintain integrity.

namespace Math
{
	class CTransformSRT;
	using CTransform = CTransformSRT;
}

namespace DEM::RPG
{
struct CItemContainerComponent;

//!!!FIXME: to config!
constexpr UPTR MAX_QUICK_SLOTS = 10;
constexpr float QUICK_SLOT_VOLUME = 1.f;

enum class EItemStorage
{
	None,      // A stack is registered as an entity but is not contained anywhere
	World,     // A stack is located as a separate object in a game scene
	Container, // A stack is stored in CItemContainerComponent
	QuickSlot, // A stack is stored in one of quick slots inside CEquipmentComponent
	Equipment  // A stack is equipped to one of equipment slots inside CEquipmentComponent
};

struct CContainerStats
{
	float  UsedWeight = 0.f;
	float  UsedVolume = 0.f;
	float  FreeVolume = 0.f;
	size_t Price = 0;
};

inline Game::HEntity GetEquippedStack(const CEquipmentComponent& Component, CStrID SlotID)
{
	auto It = Component.Equipment.find(SlotID);
	return (It == Component.Equipment.cend()) ? Game::HEntity{} : It->second;
}
//---------------------------------------------------------------------

inline bool IsStackEquipped(const CEquipmentComponent& Component, Game::HEntity StackID)
{
	for (auto [SlotID, StackInSlotID] : Component.Equipment)
		if (StackInSlotID == StackID)
			return true;
	return false;
}
//---------------------------------------------------------------------

Game::HEntity CreateItemStack(Game::CGameWorld& World, Game::HEntity ItemProtoID, U32 Count, CStrID LevelID = CStrID::Empty);
Game::HEntity CloneItemStack(Game::CGameWorld& World, Game::HEntity StackID, U32 Count);
Game::HEntity SplitItemStack(Game::CGameWorld& World, Game::HEntity StackID, U32 Count);
bool CanMergeItems(Game::HEntity ProtoID, const CItemStackComponent* pDestStack);
bool CanMergeStacks(const CItemStackComponent& SrcStack, const CItemStackComponent* pDestStack);
size_t GetFirstEmptySlotIndex(std::vector<Game::HEntity>& Collection);
void ShrinkItemCollection(std::vector<Game::HEntity>& Collection);

U32 AddItemsToContainerSlot(Game::CGameWorld& World, Game::HEntity ContainerID, size_t SlotIndex, Game::HEntity ItemProtoID, U32 Count, bool Merge = true);
U32 AddItemsToContainer(Game::CGameWorld& World, Game::HEntity ContainerID, Game::HEntity ItemProtoID, U32 Count, bool Merge = true);
std::pair<Game::HEntity, U32> MoveItemsFromContainerSlot(Game::CGameWorld& World, Game::HEntity ContainerID, size_t SlotIndex, U32 Count);
std::pair<U32, bool> MoveItemsToContainerSlot(Game::CGameWorld& World, Game::HEntity ContainerID, size_t SlotIndex, Game::HEntity StackID, U32 Count, bool Merge = true, Game::HEntity* pReplaced = nullptr);
std::pair<U32, bool> MoveItemsToContainer(Game::CGameWorld& World, Game::HEntity ContainerID, Game::HEntity StackID, U32 Count = std::numeric_limits<U32>().max(), bool Merge = true);
Game::HEntity MoveWholeStackToContainer(Game::CGameWorld& World, Game::HEntity ContainerID, Game::HEntity StackID, bool Merge = true);
void ClearContainerSlot(Game::CGameWorld& World, Game::HEntity ContainerID, size_t SlotIndex);
U32 RemoveItemsFromContainerSlot(Game::CGameWorld& World, Game::HEntity ContainerID, size_t SlotIndex, U32 Count);
U32 RemoveItemsFromContainer(Game::CGameWorld& World, Game::HEntity ContainerID, Game::HEntity ItemProtoID, U32 Count, bool AllowModified);
void CalcContainerStats(Game::CGameWorld& World, const CItemContainerComponent& Container, CContainerStats& OutStats);
bool IsContainerEmpty(const Game::CGameWorld& World, Game::HEntity ContainerID);

U32 AddItemsToQuickSlot(Game::CGameWorld& World, Game::HEntity EntityID, size_t SlotIndex, Game::HEntity ItemProtoID, U32 Count, bool Merge = true);
U32 AddItemsToQuickSlots(Game::CGameWorld& World, Game::HEntity EntityID, Game::HEntity ItemProtoID, U32 Count, bool Merge = true);
std::pair<Game::HEntity, U32> MoveItemsFromQuickSlot(Game::CGameWorld& World, Game::HEntity EntityID, size_t SlotIndex, U32 Count);
std::pair<U32, bool> MoveItemsToQuickSlot(Game::CGameWorld& World, Game::HEntity EntityID, size_t SlotIndex, Game::HEntity StackID, U32 Count, bool Merge = true, Game::HEntity* pReplaced = nullptr);
std::pair<U32, bool> MoveItemsToQuickSlots(Game::CGameWorld& World, Game::HEntity EntityID, Game::HEntity StackID, U32 Count = std::numeric_limits<U32>().max(), bool Merge = true);
Game::HEntity MoveWholeStackToQuickSlots(Game::CGameWorld& World, Game::HEntity EntityID, Game::HEntity StackID, bool Merge = true);
void ClearQuickSlot(Game::CGameWorld& World, Game::HEntity EntityID, size_t SlotIndex);
U32 RemoveItemsFromQuickSlot(Game::CGameWorld& World, Game::HEntity EntityID, size_t SlotIndex, U32 Count);
U32 RemoveItemsFromQuickSlots(Game::CGameWorld& World, Game::HEntity EntityID, Game::HEntity ItemProtoID, U32 Count, bool AllowModified);

U32 AddItemsToEquipmentSlot(Game::CGameWorld& World, Game::HEntity EntityID, CStrID SlotID, Game::HEntity ItemProtoID, U32 Count, bool Merge = true);
U32 AddItemsToEquipment(Game::CGameWorld& World, Game::HEntity EntityID, Game::HEntity ItemProtoID, U32 Count, bool Merge = true);
std::pair<Game::HEntity, U32> MoveItemsFromEquipmentSlot(Game::CGameWorld& World, Game::HEntity EntityID, CStrID SlotID, U32 Count);
std::pair<U32, bool> MoveItemsToEquipmentSlot(Game::CGameWorld& World, Game::HEntity EntityID, CStrID SlotID, Game::HEntity StackID, U32 Count, bool Merge = true, Game::HEntity* pReplaced = nullptr);
std::pair<U32, bool> MoveItemsToEquipment(Game::CGameWorld& World, Game::HEntity EntityID, Game::HEntity StackID, U32 Count, bool Merge = true);
void ClearEquipmentSlot(Game::CGameWorld& World, Game::HEntity EntityID, CStrID SlotID);
U32 RemoveItemsFromEquipmentSlot(Game::CGameWorld& World, Game::HEntity EntityID, CStrID SlotID, U32 Count);
U32 RemoveItemsFromEquipment(Game::CGameWorld& World, Game::HEntity EntityID, Game::HEntity ItemProtoID, U32 Count, bool AllowModified);
U32 CanEquipItems(const Game::CGameWorld& World, Game::HEntity ReceiverID, Game::HEntity StackID, CStrID SlotID);
void UpdateCharacterModelEquipment(Game::CGameWorld& World, Game::HEntity OwnerID, CStrID SlotID, bool ForceHide = false);
//???Game::HEntity GetEquippedStack(Game::CGameWorld& World, Game::HEntity EntityID, CStrID SlotID);
//???size_t FindEquipmentSlotForItem(item) - to check if we can equip something before we do that! E.g. for UI prompt "Equip immediately?".

U32 AddItemsToLocation(Game::CGameWorld& World, Game::HEntity ItemProtoID, U32 Count, CStrID LevelID, const Math::CTransform& Tfm, float MergeRadius);
std::pair<Game::HEntity, U32> MoveItemsFromLocation(Game::CGameWorld& World, Game::HEntity StackID, U32 Count = std::numeric_limits<U32>().max());
std::pair<U32, bool> MoveItemsToLocationSlot(Game::CGameWorld& World, std::vector<Game::HEntity>& GroundItems, size_t SlotIndex, Game::HEntity StackID, U32 Count, CStrID LevelID, const Math::CTransform& Tfm, bool Merge = true, Game::HEntity* pReplaced = nullptr);
std::pair<U32, bool> MoveItemsToLocation(Game::CGameWorld& World, Game::HEntity StackID, U32 Count, CStrID LevelID, const Math::CTransform& Tfm, float MergeRadius = 0.f);
Game::HEntity MoveWholeStackToLocation(Game::CGameWorld& World, Game::HEntity StackID, CStrID LevelID, const Math::CTransform& Tfm, float MergeRadius = 0.f);
void ClearLocationSlot(Game::CGameWorld& World, std::vector<Game::HEntity>& GroundItems, size_t SlotIndex);
U32 RemoveItemsFromLocation(Game::CGameWorld& World, Game::HEntity StackID, U32 Count);
void CleanupItemsFromLocation(Game::CGameWorld& World, Game::HEntity StackID);
bool AddItemVisualsToLocation(Game::CGameWorld& World, Game::HEntity StackID, const Math::CTransformSRT& Tfm);
void RemoveItemVisualsFromLocation(Game::CGameWorld& World, Game::HEntity StackID);

// TODO:
// void QueryItemsInShape(Game::CGameWorld& World /*collision shape - sphere, capsule etc*/ /*, T Filter*/); //!!!including piles!
//IsItemValuable, IsItemTrash - not here, game-specific filters? Or use settings from CItemManager?
//FactionHasItem(faction ID, item ID, count, allow modified)

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
