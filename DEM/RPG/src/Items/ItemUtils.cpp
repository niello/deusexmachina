#include "ItemUtils.h"
#include <Items/ItemComponent.h>
#include <Items/ItemContainerComponent.h>
#include <Items/EquipmentComponent.h>
#include <Items/EquippableComponent.h>
#include <Items/EquippedComponent.h>
#include <Items/EquipmentChangesComponent.h>
#include <Game/GameLevel.h>
#include <Scene/SceneComponent.h>
#include <Physics/RigidBodyComponent.h>

namespace DEM::RPG
{

// static
namespace
{

// TODO: to CGameLevel?
template<typename TPredicate>
static Game::HEntity FindClosestEntity(const Game::CGameLevel& Level, const rtm::vector4f& Center, float Radius, TPredicate Predicate)
{
	float ClosestDistanceSq = std::numeric_limits<float>().max();
	Game::HEntity ClosestEntityID;

	//!!!FIXME: need to set "Interactable" collision flag in all interactable entity colliders!
	Level.EnumEntitiesInSphere(Center, Radius, /*CStrID("Interactable")*/ CStrID::Empty,
		[&ClosestDistanceSq, &ClosestEntityID, Center, Predicate](Game::HEntity EntityID, const rtm::vector4f& Pos)
	{
		const auto DistanceSq = Math::vector_distance_squared3(Pos, Center);
		if (ClosestDistanceSq > DistanceSq && Predicate(EntityID))
		{
			ClosestDistanceSq = DistanceSq;
			ClosestEntityID = EntityID;
		}
		return true;
	});

	return ClosestEntityID;
}
//---------------------------------------------------------------------

U32 GetContainerCapacityInItems(const Game::CGameWorld& World, const CItemContainerComponent& Container,
	const CItemComponent* pItem, U32 MinItemCount = 1, Game::HEntity ExcludeStackID = {})
{
	if (!pItem || pItem->Volume <= 0.f || Container.MaxVolume < 0.f) return std::numeric_limits<U32>().max();

	const float MinRequiredVolume = pItem->Volume * MinItemCount;
	float FreeVolume = Container.MaxVolume;
	for (auto StoredStackID : Container.Items)
	{
		if (StoredStackID == ExcludeStackID) continue;

		auto pStoredStack = World.FindComponent<const CItemStackComponent>(StoredStackID);
		if (!pStoredStack || !pStoredStack->Count) continue;

		if (auto pStoredItem = FindItemComponent<const CItemComponent>(World, StoredStackID, *pStoredStack))
		{
			FreeVolume -= (pStoredStack->Count * pStoredItem->Volume);
			if (FreeVolume < MinRequiredVolume) return 0;
		}
	}

	// NB: float error is accumulated for every slot counted
	constexpr float MAX_INSIGNIFICANT_VOLUME = 0.0001f;
	const auto Capacity = static_cast<U32>(FreeVolume / pItem->Volume);
	const float Remainder = FreeVolume - Capacity * pItem->Volume;

	return Capacity + ((pItem->Volume - Remainder < MAX_INSIGNIFICANT_VOLUME) ? 1 : 0);
}
//---------------------------------------------------------------------

U32 GetQuickSlotCapacity(const CItemComponent* pItem)
{
	return (pItem && pItem->Volume > 0.f) ?
		static_cast<U32>(QUICK_SLOT_VOLUME / pItem->Volume) :
		std::numeric_limits<U32>().max();
}
//---------------------------------------------------------------------

size_t GetFirstMergeableSlotIndex(const Game::CGameWorld& World, const CItemContainerComponent& Container, Game::HEntity StackOrProtoID)
{
	size_t SlotIndex = 0;

	if (auto pSrcStack = World.FindComponent<const CItemStackComponent>(StackOrProtoID))
	{
		// Item stack
		for (; SlotIndex < Container.Items.size(); ++SlotIndex)
			if (auto pDestStack = World.FindComponent<const CItemStackComponent>(Container.Items[SlotIndex]))
				if (CanMergeStacks(*pSrcStack, pDestStack))
					break;
	}
	else
	{
		// Item prototype
		for (; SlotIndex < Container.Items.size(); ++SlotIndex)
			if (auto pDestStack = World.FindComponent<const CItemStackComponent>(Container.Items[SlotIndex]))
				if (CanMergeItems(StackOrProtoID, pDestStack))
					break;
	}

	return SlotIndex;
}
//---------------------------------------------------------------------

std::pair<U32, bool> MoveItemsToStack(Game::CGameWorld& World, Game::HEntity DestStackID, Game::HEntity SrcStackID, U32 Count)
{
	if (!Count || SrcStackID == DestStackID) return { Count, false };

	auto pSrcStack = World.FindComponent<CItemStackComponent>(SrcStackID);
	if (!pSrcStack) return { 0, false };

	auto pDestStack = World.FindComponent<CItemStackComponent>(DestStackID);
	if (!pDestStack) return { 0, false };

	// Handle complete transfer of the source stack
	if (pSrcStack->Count <= Count)
	{
		const auto AvailableCount = pSrcStack->Count;
		pDestStack->Count += AvailableCount;
		World.DeleteEntity(SrcStackID);
		return { AvailableCount, true };
	}

	pDestStack->Count += Count;
	pSrcStack->Count -= Count;
	return { Count, false };
}
//---------------------------------------------------------------------

inline std::pair<U32, bool> SplitItemsToSlot(Game::CGameWorld& World, Game::HEntity& DestSlot, Game::HEntity StackID, U32 Count)
{
	DestSlot = SplitItemStack(World, StackID, Count);
	if (!DestSlot) return { 0, false };
	return { Count, DestSlot == StackID };
}
//---------------------------------------------------------------------

void ClearItemCollectionSlot(std::vector<Game::HEntity>& Collection, size_t SlotIndex)
{
	if (SlotIndex >= Collection.size()) return;

	Collection[SlotIndex] = {};
	if (SlotIndex + 1 == Collection.size())
		ShrinkItemCollection(Collection);
}
//---------------------------------------------------------------------

// Returns true if the stack is emptied and deleted
bool RemoveItemsFromStack(Game::CGameWorld& World, Game::HEntity StackID, U32& Count)
{
	auto pStack = World.FindComponent<CItemStackComponent>(StackID);
	if (!pStack) return false;

	if (pStack->Count > Count)
	{
		pStack->Count -= Count;
		Count = 0;
		return false;
	}
	else
	{
		Count -= pStack->Count;
		World.DeleteEntity(StackID);
		return true;
	}
}
//---------------------------------------------------------------------

// Returns main slot ID
CStrID BlockEquipmentSlots(Game::CGameWorld& World, CEquipmentComponent& Equipment, Game::HEntity StackID)
{
	auto pEquippable = FindItemComponent<const CEquippableComponent>(World, StackID);
	if (!pEquippable || pEquippable->Slots.empty()) return CStrID::Empty;

	CStrID MainSlotID;

	// Process blocked slot types one by one
	const auto MainSlotType = pEquippable->Slots.front().first;
	for (auto [RequiredSlotType, RequiredSlotCount] : pEquippable->Slots)
	{
		if (!RequiredSlotCount) continue;

		// Count all slots already taken by this stack
		for (auto [CurrSlotID, CurrSlotType] : Equipment.Scheme->Slots)
		{
			if (CurrSlotType != RequiredSlotType) continue;

			auto It = Equipment.Equipment.find(CurrSlotID);
			if (It != Equipment.Equipment.cend() && It->second == StackID)
			{
				if (!MainSlotID && RequiredSlotType == MainSlotType) MainSlotID = CurrSlotID;
				if (--RequiredSlotCount == 0) break;
			}
		}

		if (!RequiredSlotCount) continue;

		// Block remaining number of slots
		for (auto [CurrSlotID, CurrSlotType] : Equipment.Scheme->Slots)
		{
			if (CurrSlotType != RequiredSlotType) continue;

			auto& DestSlot = Equipment.Equipment[CurrSlotID];
			if (!DestSlot)
			{
				DestSlot = StackID;
				if (!MainSlotID && RequiredSlotType == MainSlotType) MainSlotID = CurrSlotID;
				if (--RequiredSlotCount == 0) break;
			}
		}

		// Check if we failed to find enough free slots of this type. This must not happen normally.
		n_assert(!RequiredSlotCount);
	}

	return MainSlotID;
}
//---------------------------------------------------------------------

// Returns main slot ID
CStrID UnblockEquipmentSlots(Game::CGameWorld& World, CEquipmentComponent& Equipment, Game::HEntity StackID)
{
	auto pEquippable = FindItemComponent<const CEquippableComponent>(World, StackID);
	const auto MainSlotType = (pEquippable && !pEquippable->Slots.empty()) ? pEquippable->Slots.front().first : CStrID::Empty;
	CStrID MainSlotID;

	for (auto It = Equipment.Equipment.begin(); It != Equipment.Equipment.end(); /**/)
	{
		if (It->second == StackID)
		{
			if (!MainSlotID && MainSlotType == Equipment.Scheme->Slots[It->first]) MainSlotID = It->first;
			It = Equipment.Equipment.erase(It);
		}
		else
		{
			++It;
		}
	}

	return MainSlotID;
}
//---------------------------------------------------------------------

bool IsStackEquipped(const CEquipmentComponent& Component, Game::HEntity StackID)
{
	for (const auto [SlotID, StackInSlotID] : Component.Equipment)
		if (StackID == StackInSlotID)
			return true;
	return false;
}
//---------------------------------------------------------------------

// Monitored and processed by the special system
void RecordEquipment(Game::CGameWorld& World, Game::HEntity EntityID, Game::HEntity StackID, EItemStorage Storage, CStrID Slot)
{
	if (!EntityID || !StackID) return;

	if (auto pChanges = World.FindOrAddComponent<CEquipmentChangesComponent>(EntityID))
	{
		auto It = pChanges->Records.find(StackID);
		if (It == pChanges->Records.cend())
		{
			// Register the first operation for the stack
			pChanges->Records.emplace(StackID, CEquipmentChangesComponent::CRecord{ CStrID::Empty, Slot, EItemStorage::None, Storage });
		}
		else if (It->second.PrevSlot == It->second.NewSlot && It->second.PrevStorage == It->second.NewStorage)
		{
			// Turn re-equipment into more important regular record
			It->second = CEquipmentChangesComponent::CRecord{ CStrID::Empty, Slot, EItemStorage::None, Storage };
		}
		else if (It->second.PrevSlot == Slot && It->second.PrevStorage == Storage)
		{
			// Collapse the no-op record, re-equipment to the same slot must be requested explicitly with RecordReequipment()
			pChanges->Records.erase(It);
		}
		else
		{
			// Override final destination, no matter what source is
			It->second.NewSlot = Slot;
			It->second.NewStorage = Storage;
		}
	}
}
//---------------------------------------------------------------------

// Monitored and processed by the special system
void RecordUnequipment(Game::CGameWorld& World, Game::HEntity EntityID, Game::HEntity StackID, EItemStorage Storage, CStrID Slot)
{
	if (!EntityID || !StackID) return;

	if (auto pChanges = World.FindOrAddComponent<CEquipmentChangesComponent>(EntityID))
	{
		auto It = pChanges->Records.find(StackID);
		if (It == pChanges->Records.cend())
		{
			// Register the first operation for the stack
			pChanges->Records.emplace(StackID, CEquipmentChangesComponent::CRecord{ Slot, CStrID::Empty, Storage, EItemStorage::None });
		}
		else if (It->second.PrevSlot == It->second.NewSlot && It->second.PrevStorage == It->second.NewStorage)
		{
			// Turn re-equipment into more important regular record
			It->second = CEquipmentChangesComponent::CRecord{ Slot, CStrID::Empty, Storage, EItemStorage::None };
		}
		else
		{
			// Must be able to handle inverted order of operations, e.g. add to equipment -> remove from Q-slots
			if (It->second.PrevStorage == Storage)
			{
				// Override final destination
				It->second.NewSlot = CStrID::Empty;
				It->second.NewStorage = EItemStorage::None;
			}
			else if (It->second.PrevStorage == EItemStorage::None)
			{
				// Record initial source
				It->second.PrevSlot = Slot;
				It->second.PrevStorage = Storage;
			}

			if (It->second.PrevSlot == It->second.NewSlot && It->second.PrevStorage == It->second.NewStorage)
			{
				// Collapse the no-op record, re-equipment to the same slot must be requested explicitly with RecordReequipment()
				pChanges->Records.erase(It);
			}
		}
	}
}
//---------------------------------------------------------------------

}

Game::HEntity CreateItemStack(Game::CGameWorld& World, Game::HEntity ItemProtoID, U32 Count, CStrID LevelID)
{
	Game::HEntity StackID = World.CreateEntity(LevelID);
	auto pStack = World.AddComponent<CItemStackComponent>(StackID);
	if (!pStack)
	{
		World.DeleteEntity(StackID);
		return {};
	}

	pStack->Prototype = ItemProtoID;
	pStack->Count = Count;

	return StackID;
}
//---------------------------------------------------------------------

Game::HEntity CloneItemStack(Game::CGameWorld& World, Game::HEntity StackID, U32 Count)
{
	auto NewStackID = World.CloneEntityExcluding<Game::CSceneComponent, Game::CRigidBodyComponent>(StackID);
	if (!NewStackID) return {};

	if (auto pNewStack = World.FindComponent<CItemStackComponent>(NewStackID))
		pNewStack->Count = Count;

	return NewStackID;
}
//---------------------------------------------------------------------

Game::HEntity SplitItemStack(Game::CGameWorld& World, Game::HEntity StackID, U32 Count)
{
	if (!Count) return {};

	auto pSrcStack = World.FindComponent<CItemStackComponent>(StackID);
	if (!pSrcStack) return {};

	// No splitting required, move the whole source stack
	if (pSrcStack->Count <= Count) return StackID;

	auto NewStackID = World.CloneEntityExcluding<Game::CSceneComponent, Game::CRigidBodyComponent>(StackID);

	if (auto pNewStack = World.FindComponent<CItemStackComponent>(NewStackID))
	{
		pSrcStack->Count -= Count;
		pNewStack->Count = Count;
		return NewStackID;
	}

	World.DeleteEntity(NewStackID);
	return {};
}
//---------------------------------------------------------------------

bool CanMergeItems(Game::HEntity ProtoID, const CItemStackComponent* pDestStack)
{
	return pDestStack && pDestStack->Prototype == ProtoID && !pDestStack->Modified;
}
//---------------------------------------------------------------------

bool CanMergeStacks(const CItemStackComponent& SrcStack, const CItemStackComponent* pDestStack)
{
	//???TODO: don't merge items with different owners?!
	//???TODO: can add more possibilities for merging? deep comparison of components?
	return pDestStack && pDestStack->Prototype == SrcStack.Prototype && !SrcStack.Modified && !pDestStack->Modified;
}
//---------------------------------------------------------------------

size_t GetFirstEmptySlotIndex(std::vector<Game::HEntity>& Collection)
{
	size_t SlotIndex = 0;
	for (; SlotIndex < Collection.size(); ++SlotIndex)
		if (!Collection[SlotIndex])
			break;

	if (SlotIndex == Collection.size())
		Collection.push_back({});

	return SlotIndex;
}
//---------------------------------------------------------------------

//???needs to be external? or always handle here? then move to "static" namespace. Temporary ground collection code - move here?
void ShrinkItemCollection(std::vector<Game::HEntity>& Collection)
{
	// Trim the tail to the last busy slot
	auto RIt = ++Collection.rbegin();
	for (; RIt != Collection.rend(); ++RIt)
		if (*RIt) break;
	Collection.erase(RIt.base(), Collection.end());
}
//---------------------------------------------------------------------

// Returns a number of items actually added
U32 AddItemsToContainerSlot(Game::CGameWorld& World, Game::HEntity ContainerID, size_t SlotIndex, Game::HEntity ItemProtoID, U32 Count, bool Merge)
{
	if (!ContainerID || !ItemProtoID || !Count) return 0;

	auto pContainer = World.FindComponent<const CItemContainerComponent>(ContainerID);
	if (!pContainer) return 0;

	auto pItem = World.FindComponent<const CItemComponent>(ItemProtoID);
	if (!pItem) return 0;

	auto DestStackID = (SlotIndex < pContainer->Items.size()) ? pContainer->Items[SlotIndex] : Game::HEntity{};
	if (DestStackID)
	{
		if (!Merge) return 0;

		if (auto pDestStack = World.FindComponent<const CItemStackComponent>(DestStackID))
			if (!CanMergeItems(ItemProtoID, pDestStack)) return 0;
	}

	Count = std::min(Count, GetContainerCapacityInItems(World, *pContainer, pItem));
	if (!Count) return 0;

	// Now access destination for writing
	if (auto pDestStack = World.FindComponent<CItemStackComponent>(DestStackID))
	{
		pDestStack->Count += Count;
		return Count;
	}
	else if (auto NewStackID = CreateItemStack(World, ItemProtoID, Count))
	{
		if (auto pContainerWritable = World.FindComponent<CItemContainerComponent>(ContainerID))
		{
			if (SlotIndex >= pContainerWritable->Items.size())
				pContainerWritable->Items.resize(SlotIndex + 1);
			pContainerWritable->Items[SlotIndex] = NewStackID;
			return Count;
		}
		World.DeleteEntity(NewStackID);
	}

	return 0;
}
//---------------------------------------------------------------------

// Returns a number of items actually added
U32 AddItemsToContainer(Game::CGameWorld& World, Game::HEntity ContainerID, Game::HEntity ItemProtoID, U32 Count, bool Merge)
{
	if (!ContainerID || !ItemProtoID || !Count) return 0;

	auto pContainer = World.FindComponent<const CItemContainerComponent>(ContainerID);
	if (!pContainer) return 0;

	auto pItem = World.FindComponent<const CItemComponent>(ItemProtoID);
	if (!pItem) return 0;

	Count = std::min(Count, GetContainerCapacityInItems(World, *pContainer, pItem));
	if (!Count) return 0;

	// Try to merge into an existing stack first
	if (Merge)
	{
		const auto SlotIndex = GetFirstMergeableSlotIndex(World, *pContainer, ItemProtoID);
		if (SlotIndex < pContainer->Items.size())
		{
			if (auto pDestStack = World.FindComponent<CItemStackComponent>(pContainer->Items[SlotIndex]))
			{
				pDestStack->Count += Count;
				return Count;
			}
			return 0;
		}
	}

	// Put remaining count into an empty slot
	if (auto NewStackID = CreateItemStack(World, ItemProtoID, Count))
	{
		if (auto pContainerWritable = World.FindComponent<CItemContainerComponent>(ContainerID))
		{
			const auto SlotIndex = GetFirstEmptySlotIndex(pContainerWritable->Items);
			if (SlotIndex < pContainerWritable->Items.size())
			{
				pContainerWritable->Items[SlotIndex] = NewStackID;
				return Count;
			}
		}
		World.DeleteEntity(NewStackID);
	}

	return 0;
}
//---------------------------------------------------------------------

// Returns a source stack ID if movement happened and a number of moved items (zero if the whole stack is moved)
std::pair<Game::HEntity, U32> MoveItemsFromContainerSlot(Game::CGameWorld& World, Game::HEntity ContainerID, size_t SlotIndex, U32 Count)
{
	if (!ContainerID || !Count) return { {}, 0 };

	auto pContainer = World.FindComponent<const CItemContainerComponent>(ContainerID);
	if (!pContainer || SlotIndex >= pContainer->Items.size()) return { {}, 0 };

	// Don't optimize with constant access because a stack will be altered or deleted anyway
	const auto StackID = pContainer->Items[SlotIndex];
	auto pStack = World.FindComponent<CItemStackComponent>(StackID);
	if (!pStack) return { {}, 0 };

	if (pStack->Count > Count)
	{
		pStack->Count -= Count;
		return { StackID, Count };
	}
	else
	{
		if (auto pContainerWritable = World.FindComponent<CItemContainerComponent>(ContainerID))
		{
			pContainerWritable->Items[SlotIndex] = {};
			if (SlotIndex + 1 == pContainerWritable->Items.size())
				ShrinkItemCollection(pContainerWritable->Items);
		}
		return { StackID, 0 };
	}
}
//---------------------------------------------------------------------

// Returns a number of items actually moved in and a 'need to clear source storage' flag
std::pair<U32, bool> MoveItemsToContainerSlot(Game::CGameWorld& World, Game::HEntity ContainerID, size_t SlotIndex, Game::HEntity StackID,
	U32 Count, bool Merge, Game::HEntity* pReplaced)
{
	if (!ContainerID || !StackID || !Count) return { 0, false };

	auto pContainer = World.FindComponent<const CItemContainerComponent>(ContainerID);
	if (!pContainer) return { 0, false };
	auto& Items = pContainer->Items;

	Game::HEntity DestStackID;
	if (SlotIndex < Items.size())
	{
		DestStackID = Items[SlotIndex];
		if (StackID == DestStackID) return { Count, false };
		if (!Merge && !*pReplaced && DestStackID) return { 0, false };
	}

	auto pSrcStack = World.FindComponent<const CItemStackComponent>(StackID);
	if (!pSrcStack || !pSrcStack->Count) return { 0, false };

	auto pItem = FindItemComponent<const CItemComponent>(World, StackID, *pSrcStack);
	if (!pItem) return { 0, false };

	// Check if the stack is moved within the same storage
	const auto FoundIndex = static_cast<size_t>(std::distance(Items.cbegin(), std::find(Items.cbegin(), Items.cend(), StackID)));
	const auto IsInternalMove = (FoundIndex < Items.size());

	const auto Capacity = IsInternalMove ? pSrcStack->Count : GetContainerCapacityInItems(World, *pContainer, pItem, 1, StackID);
	Count = std::min({ Count, pSrcStack->Count, Capacity });
	if (!Count) return { 0, false };

	// Consider destination occupied only if the destination stack is valid
	if (auto pDestStack = World.FindComponent<const CItemStackComponent>(DestStackID))
	{
		if (Merge && CanMergeStacks(*pSrcStack, pDestStack))
		{
			const auto [MovedCount, MovedCompletely] = MoveItemsToStack(World, DestStackID, StackID, Count);
			if (MovedCompletely && IsInternalMove) ClearContainerSlot(World, ContainerID, FoundIndex);
			return { MovedCount, MovedCompletely };
		}

		if (!pReplaced) return { 0, false };
	}

	if (auto pContainerWritable = World.FindComponent<CItemContainerComponent>(ContainerID))
	{
		auto& WritableItems = pContainerWritable->Items;

		if (SlotIndex >= WritableItems.size())
			WritableItems.resize(SlotIndex + 1);

		const auto [MovedCount, MovedCompletely] = SplitItemsToSlot(World, WritableItems[SlotIndex], StackID, Count);
		if (MovedCount)
		{
			if (pReplaced) *pReplaced = DestStackID;
			if (MovedCompletely && IsInternalMove) ClearItemCollectionSlot(WritableItems, FoundIndex);
			return { MovedCount, MovedCompletely };
		}
	}

	return { 0, false };
}
//---------------------------------------------------------------------

// Returns a number of items actually moved in and a 'need to clear source storage' flag
std::pair<U32, bool> MoveItemsToContainer(Game::CGameWorld& World, Game::HEntity ContainerID, Game::HEntity StackID, U32 Count, bool Merge)
{
	if (!ContainerID || !StackID || !Count) return { 0, false };

	auto pContainer = World.FindComponent<const CItemContainerComponent>(ContainerID);
	if (!pContainer) return { 0, false };

	// Skip if the stack is already contained in this storage
	if (std::find(pContainer->Items.cbegin(), pContainer->Items.cend(), StackID) != pContainer->Items.cend()) return { Count, false };

	auto pSrcStack = World.FindComponent<const CItemStackComponent>(StackID);
	if (!pSrcStack || !pSrcStack->Count) return { 0, false };

	auto pItem = FindItemComponent<const CItemComponent>(World, StackID, *pSrcStack);
	if (!pItem) return { 0, false };

	Count = std::min({ Count, pSrcStack->Count, GetContainerCapacityInItems(World, *pContainer, pItem, 1, StackID) });
	if (Count < 1) return { 0, false };

	// Try to merge into an existing stack first
	if (Merge)
	{
		const auto SlotIndex = GetFirstMergeableSlotIndex(World, *pContainer, StackID);
		if (SlotIndex < pContainer->Items.size())
			return MoveItemsToStack(World, pContainer->Items[SlotIndex], StackID, Count);
	}

	// Put remaining count into an empty slot
	if (auto pContainerWritable = World.FindComponent<CItemContainerComponent>(ContainerID))
	{
		const auto SlotIndex = GetFirstEmptySlotIndex(pContainerWritable->Items);
		if (SlotIndex < pContainerWritable->Items.size())
			return SplitItemsToSlot(World, pContainerWritable->Items[SlotIndex], StackID, Count);
	}

	return { 0, false };
}
//---------------------------------------------------------------------

Game::HEntity MoveWholeStackToContainer(Game::CGameWorld& World, Game::HEntity ContainerID, Game::HEntity StackID, bool Merge)
{
	if (!ContainerID || !StackID) return {};

	auto pContainer = World.FindComponent<const CItemContainerComponent>(ContainerID);
	if (!pContainer) return {};

	// Skip if the stack is already contained in this storage
	if (std::find(pContainer->Items.cbegin(), pContainer->Items.cend(), StackID) != pContainer->Items.cend()) return StackID;

	auto pSrcStack = World.FindComponent<const CItemStackComponent>(StackID);
	if (!pSrcStack || !pSrcStack->Count) return {};

	auto pItem = FindItemComponent<const CItemComponent>(World, StackID, *pSrcStack);
	if (!pItem) return {};

	if (pSrcStack->Count > GetContainerCapacityInItems(World, *pContainer, pItem, 1, StackID)) return {};

	// Try to merge into an existing stack first
	if (Merge)
	{
		const auto SlotIndex = GetFirstMergeableSlotIndex(World, *pContainer, StackID);
		if (SlotIndex < pContainer->Items.size())
		{
			if (!MoveItemsToStack(World, pContainer->Items[SlotIndex], StackID, pSrcStack->Count).first) return {};
			return pContainer->Items[SlotIndex];
		}
	}

	// Put remaining count into an empty slot
	if (auto pContainerWritable = World.FindComponent<CItemContainerComponent>(ContainerID))
	{
		const auto SlotIndex = GetFirstEmptySlotIndex(pContainerWritable->Items);
		if (SlotIndex < pContainerWritable->Items.size())
		{
			if (!SplitItemsToSlot(World, pContainerWritable->Items[SlotIndex], StackID, pSrcStack->Count).first) return {};
			return pContainerWritable->Items[SlotIndex];
		}
	}

	return {};
}
//---------------------------------------------------------------------

void ClearContainerSlot(Game::CGameWorld& World, Game::HEntity ContainerID, size_t SlotIndex)
{
	if (auto pContainer = World.FindComponent<CItemContainerComponent>(ContainerID))
		ClearItemCollectionSlot(pContainer->Items, SlotIndex);
}
//---------------------------------------------------------------------

// Returns a number of items actually removed
U32 RemoveItemsFromContainerSlot(Game::CGameWorld& World, Game::HEntity ContainerID, size_t SlotIndex, U32 Count)
{
	const auto [StackID, RemovedCount] = MoveItemsFromContainerSlot(World, ContainerID, SlotIndex, Count);
	if (StackID && !RemovedCount) World.DeleteEntity(StackID);
	return RemovedCount;
}
//---------------------------------------------------------------------

// Returns a number of items actually removed
U32 RemoveItemsFromContainer(Game::CGameWorld& World, Game::HEntity ContainerID, Game::HEntity ItemProtoID, U32 Count, bool AllowModified)
{
	if (!ContainerID || !ItemProtoID || !Count) return 0;

	auto pContainer = World.FindComponent<CItemContainerComponent>(ContainerID);
	if (!pContainer) return 0;

	U32 RemainingCount = Count;
	for (auto& StackID : pContainer->Items)
	{
		auto pStack = World.FindComponent<const CItemStackComponent>(StackID);
		if (!pStack || pStack->Prototype != ItemProtoID || (!AllowModified && pStack->Modified)) continue;

		if (RemoveItemsFromStack(World, StackID, RemainingCount))
			StackID = {};

		if (!RemainingCount) break;
	}

	ShrinkItemCollection(pContainer->Items);

	return Count - RemainingCount;
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

	OutStats.FreeVolume = (Container.MaxVolume <= 0.f) ? FLT_MAX : (Container.MaxVolume - OutStats.UsedVolume);
}
//---------------------------------------------------------------------

bool IsContainerEmpty(const Game::CGameWorld& World, Game::HEntity ContainerID)
{
	const CItemContainerComponent* pContainer = World.FindComponent<const CItemContainerComponent>(ContainerID);
	if (!pContainer) return true;

	for (auto StackID : pContainer->Items)
	{
		auto pItemStack = World.FindComponent<const CItemStackComponent>(StackID);
		if (pItemStack && pItemStack->Prototype && pItemStack->Count > 0) return false;
	}

	return true;
}
//---------------------------------------------------------------------

// FIXME: create once, return from mapping? Can precreate!
CStrID GetQuickSlotID(size_t SlotIndex)
{
	const std::string ID = "Q" + std::to_string(SlotIndex + 1);
	return CStrID(ID.c_str());
}
//---------------------------------------------------------------------

// Returns a number of items actually added
U32 AddItemsToQuickSlot(Game::CGameWorld& World, Game::HEntity EntityID, size_t SlotIndex, Game::HEntity ItemProtoID, U32 Count, bool Merge)
{
	if (!EntityID || !ItemProtoID || !Count) return 0;

	auto pEquipment = World.FindComponent<const CEquipmentComponent>(EntityID);
	if (!pEquipment || SlotIndex >= pEquipment->QuickSlots.size()) return 0;

	auto pItem = World.FindComponent<const CItemComponent>(ItemProtoID);
	if (!pItem) return 0;

	U32 SlotCapacity = GetQuickSlotCapacity(pItem);
	if (!SlotCapacity) return 0;

	auto DestStackID = pEquipment->QuickSlots[SlotIndex];

	// Can't add to a slot occupied by an incompatible item stack
	// NB: dest stack is accessed for reading
	if (DestStackID)
	{
		if (!Merge) return 0;

		if (auto pDestStack = World.FindComponent<const CItemStackComponent>(DestStackID))
		{
			if (SlotCapacity <= pDestStack->Count || !CanMergeItems(ItemProtoID, pDestStack)) return 0;
			SlotCapacity -= pDestStack->Count;
		}
	}

	Count = std::min(Count, SlotCapacity);

	// Now access destination for writing
	if (auto pDestStack = World.FindComponent<CItemStackComponent>(DestStackID))
	{
		pDestStack->Count += Count;
		return Count;
	}
	else if (auto NewStackID = CreateItemStack(World, ItemProtoID, Count))
	{
		if (auto pEquipmentWritable = World.FindComponent<CEquipmentComponent>(EntityID))
		{
			pEquipmentWritable->QuickSlots[SlotIndex] = NewStackID;
			RecordEquipment(World, EntityID, NewStackID, EItemStorage::QuickSlot, GetQuickSlotID(SlotIndex));
			return Count;
		}
		World.DeleteEntity(NewStackID);
	}

	return 0;
}
//---------------------------------------------------------------------

// Returns a number of items actually added
U32 AddItemsToQuickSlots(Game::CGameWorld& World, Game::HEntity EntityID, Game::HEntity ItemProtoID, U32 Count, bool Merge)
{
	if (!EntityID || !ItemProtoID || !Count) return 0;

	auto pEquipment = World.FindComponent<const CEquipmentComponent>(EntityID);
	if (!pEquipment) return 0;

	auto pItem = World.FindComponent<const CItemComponent>(ItemProtoID);
	if (!pItem) return 0;

	const U32 SlotCapacity = GetQuickSlotCapacity(pItem);
	if (!SlotCapacity) return 0;

	U32 RemainingCount = Count;

	// First try to merge into existing stacks of the same item
	if (Merge)
	{
		for (auto DestStackID : pEquipment->QuickSlots)
		{
			auto pDestStack = World.FindComponent<const CItemStackComponent>(DestStackID);
			if (!pDestStack || pDestStack->Count >= SlotCapacity || !CanMergeItems(ItemProtoID, pDestStack)) continue;

			if (auto pDestStackWritable = World.FindComponent<CItemStackComponent>(DestStackID))
			{
				const auto RemainingCapacity = SlotCapacity - pDestStack->Count;
				pDestStackWritable->Count += std::min(RemainingCapacity, RemainingCount);
				if (RemainingCapacity >= RemainingCount) return Count;
				RemainingCount -= RemainingCapacity;
			}
		}
	}

	// Put remaining count into free slots
	if (auto pEquipmentWritable = World.FindComponent<CEquipmentComponent>(EntityID))
	{
		for (size_t SlotIndex = 0; SlotIndex < pEquipmentWritable->QuickSlots.size(); ++SlotIndex)
		{
			auto& DestSlot = pEquipmentWritable->QuickSlots[SlotIndex];
			if (DestSlot) continue;
			DestSlot = CreateItemStack(World, ItemProtoID, std::min(SlotCapacity, RemainingCount));
			RecordEquipment(World, EntityID, DestSlot, EItemStorage::QuickSlot, GetQuickSlotID(SlotIndex));
			if (SlotCapacity >= RemainingCount) return Count;
			RemainingCount -= SlotCapacity;
		}
	}

	return Count - RemainingCount;
}
//---------------------------------------------------------------------

// Returns a source stack ID if movement happened and a number of moved items (zero if the whole stack is moved)
std::pair<Game::HEntity, U32> MoveItemsFromQuickSlot(Game::CGameWorld& World, Game::HEntity EntityID, size_t SlotIndex, U32 Count)
{
	if (!EntityID || !Count) return { {}, 0 };

	auto pEquipment = World.FindComponent<const CEquipmentComponent>(EntityID);
	if (!pEquipment || SlotIndex >= pEquipment->QuickSlots.size()) return { {}, 0 };

	// Don't optimize with constant access because a stack will be altered or deleted anyway
	const auto StackID = pEquipment->QuickSlots[SlotIndex];
	auto pStack = World.FindComponent<CItemStackComponent>(StackID);
	if (!pStack) return { {}, 0 };

	if (pStack->Count > Count)
	{
		pStack->Count -= Count;
		return { StackID, Count };
	}
	else
	{
		if (auto pEquipmentWritable = World.FindComponent<CEquipmentComponent>(EntityID))
		{
			pEquipmentWritable->QuickSlots[SlotIndex] = {};
			RecordUnequipment(World, EntityID, StackID, EItemStorage::QuickSlot, GetQuickSlotID(SlotIndex));
		}
		return { StackID, 0 };
	}
}
//---------------------------------------------------------------------

// Returns a number of items actually moved in and a 'need to clear source storage' flag
std::pair<U32, bool> MoveItemsToQuickSlot(Game::CGameWorld& World, Game::HEntity EntityID, size_t SlotIndex, Game::HEntity StackID,
	U32 Count, bool Merge, Game::HEntity* pReplaced)
{
	if (!EntityID || !StackID || !Count) return { 0, false };

	auto pEquipment = World.FindComponent<const CEquipmentComponent>(EntityID);
	if (!pEquipment || SlotIndex >= pEquipment->QuickSlots.size()) return { 0, false };

	const auto DestStackID = pEquipment->QuickSlots[SlotIndex];
	if (StackID == DestStackID) return { Count, false };
	if (!Merge && !*pReplaced && DestStackID) return { 0, false };

	auto pSrcStack = World.FindComponent<const CItemStackComponent>(StackID);
	if (!pSrcStack || !pSrcStack->Count) return { 0, false };

	auto pItem = FindItemComponent<const CItemComponent>(World, StackID, *pSrcStack);
	if (!pItem) return { 0, false };

	const U32 SlotCapacity = GetQuickSlotCapacity(pItem);
	if (!SlotCapacity) return { 0, false };

	// Check if the stack is moved within the same storage
	const auto FoundIndex = static_cast<size_t>(std::distance(pEquipment->QuickSlots.cbegin(), std::find(pEquipment->QuickSlots.cbegin(), pEquipment->QuickSlots.cend(), StackID)));
	const auto IsInternalMove = (FoundIndex < pEquipment->QuickSlots.size());

	if (auto pDestStack = World.FindComponent<const CItemStackComponent>(DestStackID))
	{
		if (Merge && pDestStack->Count < SlotCapacity && CanMergeStacks(*pSrcStack, pDestStack))
		{
			const auto [MovedCount, MovedCompletely] = MoveItemsToStack(World, DestStackID, StackID, std::min(Count, SlotCapacity - pDestStack->Count));
			if (MovedCompletely && IsInternalMove)
			{
				if (auto pEquipmentWritable = World.FindComponent<CEquipmentComponent>(EntityID))
					pEquipmentWritable->QuickSlots[FoundIndex] = {};
				RecordUnequipment(World, EntityID, StackID, EItemStorage::QuickSlot, GetQuickSlotID(FoundIndex));
			}
			return { MovedCount, MovedCompletely };
		}

		if (!pReplaced) return { 0, false };
	}

	if (auto pEquipmentWritable = World.FindComponent<CEquipmentComponent>(EntityID))
	{
		const auto [MovedCount, MovedCompletely] = SplitItemsToSlot(World, pEquipmentWritable->QuickSlots[SlotIndex], StackID, std::min(Count, SlotCapacity));
		if (MovedCount)
		{
			if (pReplaced)
			{
				*pReplaced = DestStackID;
				RecordUnequipment(World, EntityID, DestStackID, EItemStorage::QuickSlot, GetQuickSlotID(SlotIndex));
			}

			if (MovedCompletely && IsInternalMove)
			{
				pEquipmentWritable->QuickSlots[FoundIndex] = {};
				RecordUnequipment(World, EntityID, StackID, EItemStorage::QuickSlot, GetQuickSlotID(FoundIndex));
			}

			RecordEquipment(World, EntityID, StackID, EItemStorage::QuickSlot, GetQuickSlotID(SlotIndex));
			return { MovedCount, MovedCompletely };
		}
	}

	return { 0, false };
}
//---------------------------------------------------------------------

// Returns a number of items actually moved in and a 'need to clear source storage' flag
std::pair<U32, bool> MoveItemsToQuickSlots(Game::CGameWorld& World, Game::HEntity EntityID, Game::HEntity StackID, U32 Count, bool Merge)
{
	if (!EntityID || !StackID || !Count) return { 0, false };

	auto pEquipment = World.FindComponent<const CEquipmentComponent>(EntityID);
	if (!pEquipment) return { 0, false };

	// Skip if the stack is already contained in this storage
	if (std::find(pEquipment->QuickSlots.cbegin(), pEquipment->QuickSlots.cend(), StackID) != pEquipment->QuickSlots.cend()) return { Count, false };

	auto pSrcStack = World.FindComponent<const CItemStackComponent>(StackID);
	if (!pSrcStack || !pSrcStack->Count) return { 0, false };

	auto pItem = FindItemComponent<const CItemComponent>(World, StackID, *pSrcStack);
	if (!pItem) return { 0, false };

	const auto SlotCapacity = GetQuickSlotCapacity(pItem);
	if (!SlotCapacity) return { 0, false };

	if (Count > pSrcStack->Count) Count = pSrcStack->Count;
	U32 RemainingCount = Count;

	// First try to merge into existing stacks of the same item
	if (Merge)
	{
		for (auto DestStackID : pEquipment->QuickSlots)
		{
			auto pDestStack = World.FindComponent<const CItemStackComponent>(DestStackID);
			if (!pDestStack || pDestStack->Count >= SlotCapacity || !CanMergeStacks(*pSrcStack, pDestStack)) continue;

			const auto [MovedCount, MovedCompletely] = MoveItemsToStack(World, DestStackID, StackID, std::min(RemainingCount, SlotCapacity - pDestStack->Count));
			if (MovedCount >= RemainingCount) return { Count, MovedCompletely };
			RemainingCount -= MovedCount;
		}
	}

	// Put remaining count into free slots
	if (auto pEquipmentWritable = World.FindComponent<CEquipmentComponent>(EntityID))
	{
		for (size_t SlotIndex = 0; SlotIndex < pEquipmentWritable->QuickSlots.size(); ++SlotIndex)
		{
			auto& DestSlot = pEquipmentWritable->QuickSlots[SlotIndex];
			if (DestSlot) continue;
			const auto [MovedCount, MovedCompletely] = SplitItemsToSlot(World, DestSlot, StackID, std::min(RemainingCount, SlotCapacity));
			RecordEquipment(World, EntityID, DestSlot, EItemStorage::QuickSlot, GetQuickSlotID(SlotIndex));
			if (MovedCount >= RemainingCount) return { Count, MovedCompletely };
			RemainingCount -= MovedCount;
		}
	}

	return { Count - RemainingCount, false };
}
//---------------------------------------------------------------------

Game::HEntity MoveWholeStackToQuickSlots(Game::CGameWorld& World, Game::HEntity EntityID, Game::HEntity StackID, bool Merge)
{
	if (!EntityID || !StackID) return {};

	auto pEquipment = World.FindComponent<const CEquipmentComponent>(EntityID);
	if (!pEquipment) return {};

	// Skip if the stack is already contained in this storage
	if (std::find(pEquipment->QuickSlots.cbegin(), pEquipment->QuickSlots.cend(), StackID) != pEquipment->QuickSlots.cend()) return StackID;

	auto pSrcStack = World.FindComponent<const CItemStackComponent>(StackID);
	if (!pSrcStack || !pSrcStack->Count) return {};

	auto pItem = FindItemComponent<const CItemComponent>(World, StackID, *pSrcStack);
	if (!pItem) return {};

	const auto SlotCapacity = GetQuickSlotCapacity(pItem);
	if (pSrcStack->Count > SlotCapacity) return {};

	// First try to merge into existing stacks of the same item
	if (Merge)
	{
		for (auto DestStackID : pEquipment->QuickSlots)
		{
			auto pDestStack = World.FindComponent<const CItemStackComponent>(DestStackID);
			if (!pDestStack || (pSrcStack->Count + pDestStack->Count > SlotCapacity) || !CanMergeStacks(*pSrcStack, pDestStack)) continue;
			if (!MoveItemsToStack(World, DestStackID, StackID, pSrcStack->Count).first) return {};
			return DestStackID;
		}
	}

	// Put remaining count into free slots
	if (auto pEquipmentWritable = World.FindComponent<CEquipmentComponent>(EntityID))
	{
		for (size_t SlotIndex = 0; SlotIndex < pEquipmentWritable->QuickSlots.size(); ++SlotIndex)
		{
			auto& DestSlot = pEquipmentWritable->QuickSlots[SlotIndex];
			if (DestSlot) continue;
			if (!SplitItemsToSlot(World, DestSlot, StackID, pSrcStack->Count).first) return {};
			RecordEquipment(World, EntityID, DestSlot, EItemStorage::QuickSlot, GetQuickSlotID(SlotIndex));
			return DestSlot;
		}
	}

	return {};
}
//---------------------------------------------------------------------

void ClearQuickSlot(Game::CGameWorld& World, Game::HEntity EntityID, size_t SlotIndex)
{
	auto pEquipment = World.FindComponent<CEquipmentComponent>(EntityID);
	if (!pEquipment || SlotIndex >= pEquipment->QuickSlots.size()) return;
	const auto StackID = pEquipment->QuickSlots[SlotIndex];
	pEquipment->QuickSlots[SlotIndex] = {};
	RecordUnequipment(World, EntityID, StackID, EItemStorage::QuickSlot, GetQuickSlotID(SlotIndex));
}
//---------------------------------------------------------------------

// Returns a number of items actually removed
U32 RemoveItemsFromQuickSlot(Game::CGameWorld& World, Game::HEntity EntityID, size_t SlotIndex, U32 Count)
{
	const auto [StackID, RemovedCount] = MoveItemsFromQuickSlot(World, EntityID, SlotIndex, Count);
	if (StackID && !RemovedCount) World.DeleteEntity(StackID);
	return RemovedCount;
}
//---------------------------------------------------------------------

// Returns a number of items actually removed
U32 RemoveItemsFromQuickSlots(Game::CGameWorld& World, Game::HEntity EntityID, Game::HEntity ItemProtoID, U32 Count, bool AllowModified)
{
	if (!EntityID || !ItemProtoID || !Count) return 0;

	auto pEquipment = World.FindComponent<CEquipmentComponent>(EntityID);
	if (!pEquipment) return 0;

	U32 RemainingCount = Count;
	for (size_t SlotIndex = 0; SlotIndex < pEquipment->QuickSlots.size(); ++SlotIndex)
	{
		const auto StackID = pEquipment->QuickSlots[SlotIndex];
		auto pStack = World.FindComponent<const CItemStackComponent>(StackID);
		if (!pStack || pStack->Prototype != ItemProtoID || (!AllowModified && pStack->Modified)) continue;

		if (RemoveItemsFromStack(World, StackID, RemainingCount))
		{
			pEquipment->QuickSlots[SlotIndex] = {};
			RecordUnequipment(World, EntityID, StackID, EItemStorage::QuickSlot, GetQuickSlotID(SlotIndex));
		}

		if (!RemainingCount) break;
	}

	return Count - RemainingCount;
}
//---------------------------------------------------------------------

// FIXME: major code duplication with AddItemsToQuickSlot!
// Returns a number of items actually added
U32 AddItemsToEquipmentSlot(Game::CGameWorld& World, Game::HEntity EntityID, CStrID SlotID, Game::HEntity ItemProtoID, U32 Count, bool Merge)
{
	if (!EntityID || !ItemProtoID || !Count) return 0;

	auto pEquipment = World.FindComponent<const CEquipmentComponent>(EntityID);
	if (!pEquipment) return 0;

	U32 SlotCapacity = CanEquipItems(World, EntityID, ItemProtoID, SlotID);
	if (!SlotCapacity) return 0;

	auto DestStackID = pEquipment->Equipment.at(SlotID);

	// Can't add to a slot occupied by an incompatible item stack
	// NB: dest stack is accessed for reading
	if (DestStackID)
	{
		if (!Merge) return 0;

		if (auto pDestStack = World.FindComponent<const CItemStackComponent>(DestStackID))
		{
			if (SlotCapacity <= pDestStack->Count || !CanMergeItems(ItemProtoID, pDestStack)) return 0;
			SlotCapacity -= pDestStack->Count;
		}
	}

	Count = std::min(Count, SlotCapacity);

	// Now access destination for writing
	if (auto pDestStack = World.FindComponent<CItemStackComponent>(DestStackID))
	{
		pDestStack->Count += Count;
		return Count;
	}
	else if (auto NewStackID = CreateItemStack(World, ItemProtoID, Count))
	{
		if (auto pEquipmentWritable = World.FindComponent<CEquipmentComponent>(EntityID))
		{
			pEquipmentWritable->Equipment.emplace(SlotID, NewStackID);
			const auto MainSlotID = BlockEquipmentSlots(World, *pEquipmentWritable, NewStackID);
			RecordEquipment(World, EntityID, NewStackID, EItemStorage::Equipment, MainSlotID);
			return Count;
		}
		World.DeleteEntity(NewStackID);
	}

	return 0;
}
//---------------------------------------------------------------------

// Returns a number of items actually added
U32 AddItemsToEquipment(Game::CGameWorld& World, Game::HEntity EntityID, Game::HEntity ItemProtoID, U32 Count, bool Merge)
{
	if (!EntityID || !ItemProtoID || !Count) return 0;

	auto pEquipment = World.FindComponent<const CEquipmentComponent>(EntityID);
	if (!pEquipment) return 0;

	U32 RemainingCount = Count;

	// First try to merge into existing stacks of the same item
	if (Merge)
	{
		for (auto [SlotID, SlotType] : pEquipment->Scheme->Slots)
		{
			auto It = pEquipment->Equipment.find(SlotID);
			if (It == pEquipment->Equipment.cend()) continue;

			auto pDestStack = World.FindComponent<const CItemStackComponent>(It->second);
			if (!pDestStack || !CanMergeItems(ItemProtoID, pDestStack)) continue;

			const U32 SlotCapacity = CanEquipItems(World, EntityID, ItemProtoID, SlotID);
			if (pDestStack->Count >= SlotCapacity) continue;

			if (auto pDestStackWritable = World.FindComponent<CItemStackComponent>(It->second))
			{
				const auto RemainingCapacity = SlotCapacity - pDestStack->Count;
				pDestStackWritable->Count += std::min(RemainingCapacity, RemainingCount);
				if (RemainingCapacity >= RemainingCount) return Count;
				RemainingCount -= RemainingCapacity;
			}
		}
	}

	// Put remaining count into free slots
	if (auto pEquipmentWritable = World.FindComponent<CEquipmentComponent>(EntityID))
	{
		for (auto [SlotID, SlotType] : pEquipmentWritable->Scheme->Slots)
		{
			auto& DestSlot = pEquipmentWritable->Equipment[SlotID];
			if (DestSlot) continue;
			const U32 SlotCapacity = CanEquipItems(World, EntityID, ItemProtoID, SlotID);
			DestSlot = CreateItemStack(World, ItemProtoID, std::min(SlotCapacity, RemainingCount));
			const auto MainSlotID = BlockEquipmentSlots(World, *pEquipmentWritable, DestSlot);
			RecordEquipment(World, EntityID, DestSlot, EItemStorage::Equipment, MainSlotID);
			if (SlotCapacity >= RemainingCount) return Count;
			RemainingCount -= SlotCapacity;
		}
	}

	return Count - RemainingCount;
}
//---------------------------------------------------------------------

// Returns a source stack ID if movement happened and a number of moved items (zero if the whole stack is moved)
std::pair<Game::HEntity, U32> MoveItemsFromEquipmentSlot(Game::CGameWorld& World, Game::HEntity EntityID, CStrID SlotID, U32 Count)
{
	if (!EntityID || !Count) return { {}, 0 };

	auto pEquipment = World.FindComponent<const CEquipmentComponent>(EntityID);
	if (!pEquipment) return { {}, 0 };

	auto It = pEquipment->Equipment.find(SlotID);
	if (It == pEquipment->Equipment.cend()) return { {}, 0 };

	// Don't optimize with constant access because a stack will be altered or deleted anyway
	const auto StackID = It->second;
	auto pStack = World.FindComponent<CItemStackComponent>(StackID);
	if (!pStack) return { {}, 0 };

	if (pStack->Count > Count)
	{
		pStack->Count -= Count;
		return { StackID, Count };
	}
	else
	{
		if (auto pEquipmentWritable = World.FindComponent<CEquipmentComponent>(EntityID))
		{
			const auto MainSlotID = UnblockEquipmentSlots(World, *pEquipmentWritable, StackID);
			RecordUnequipment(World, EntityID, StackID, EItemStorage::Equipment, MainSlotID);
		}
		return { StackID, 0 };
	}
}
//---------------------------------------------------------------------

// Returns a number of items actually moved in and a 'need to clear source storage' flag
std::pair<U32, bool> MoveItemsToEquipmentSlot(Game::CGameWorld& World, Game::HEntity EntityID, CStrID SlotID, Game::HEntity StackID,
	U32 Count, bool Merge, Game::HEntity* pReplaced)
{
	if (!EntityID || !StackID || !Count) return { 0, false };

	auto pEquipment = World.FindComponent<const CEquipmentComponent>(EntityID);
	if (!pEquipment) return { 0, false };

	const U32 SlotCapacity = CanEquipItems(World, EntityID, StackID, SlotID);
	if (!SlotCapacity) return { 0, false };

	const auto DestStackID = GetEquippedStack(*pEquipment, SlotID);

	if (StackID == DestStackID) return { Count, false };
	if (!Merge && !*pReplaced && DestStackID) return { 0, false };

	auto pSrcStack = World.FindComponent<const CItemStackComponent>(StackID);
	if (!pSrcStack || !pSrcStack->Count) return { 0, false };

	// Check if the stack is moved within the same storage
	const auto IsAlreadyEquipped = IsStackEquipped(*pEquipment, StackID);

	// Try to merge items to the existing stack
	if (auto pDestStack = World.FindComponent<const CItemStackComponent>(DestStackID))
	{
		if (Merge && pDestStack->Count < SlotCapacity && CanMergeStacks(*pSrcStack, pDestStack))
		{
			const auto [MovedCount, MovedCompletely] = MoveItemsToStack(World, DestStackID, StackID, std::min(Count, SlotCapacity - pDestStack->Count));
			if (MovedCompletely && IsAlreadyEquipped)
			{
				if (auto pEquipmentWritable = World.FindComponent<CEquipmentComponent>(EntityID))
				{
					const auto MainSlotID = UnblockEquipmentSlots(World, *pEquipmentWritable, StackID);
					RecordUnequipment(World, EntityID, StackID, EItemStorage::Equipment, MainSlotID);
				}
			}
			return { MovedCount, MovedCompletely };
		}

		// Fail if the target slot is not empty and replacement is not allowed
		if (!pReplaced) return { 0, false };
	}

	// Put our stack to the slot, replacing prevoius contents if needed
	if (auto pEquipmentWritable = World.FindComponent<CEquipmentComponent>(EntityID))
	{
		const auto [MovedCount, MovedCompletely] = SplitItemsToSlot(World, pEquipmentWritable->Equipment[SlotID], StackID, std::min(Count, SlotCapacity));
		if (MovedCount)
		{
			const auto FinalStackID = pEquipmentWritable->Equipment[SlotID];

			// Replaced item is no longer equipped
			if (pReplaced)
			{
				*pReplaced = DestStackID;
				const auto MainSlotID = UnblockEquipmentSlots(World, *pEquipmentWritable, DestStackID);
				RecordUnequipment(World, EntityID, DestStackID, EItemStorage::Equipment, MainSlotID);
			}

			// Clear all blocked slots except explicitly chosen, they can be incorrect after item moving
			if (MovedCompletely && IsAlreadyEquipped)
			{
				n_assert_dbg(FinalStackID == StackID);
				UnblockEquipmentSlots(World, *pEquipmentWritable, StackID);
				pEquipmentWritable->Equipment[SlotID] = FinalStackID;
			}

			// Redo blocking of additional slots from scratch, now they will be correct
			const auto MainSlotID = BlockEquipmentSlots(World, *pEquipmentWritable, FinalStackID);
			RecordEquipment(World, EntityID, FinalStackID, EItemStorage::Equipment, MainSlotID);
			return { MovedCount, MovedCompletely };
		}
	}

	return { 0, false };
}
//---------------------------------------------------------------------

// Returns a number of items actually moved in and a 'need to clear source storage' flag
std::pair<U32, bool> MoveItemsToEquipment(Game::CGameWorld& World, Game::HEntity EntityID, Game::HEntity StackID, U32 Count, bool Merge)
{
	if (!EntityID || !StackID || !Count) return { 0, false };

	auto pEquipment = World.FindComponent<const CEquipmentComponent>(EntityID);
	if (!pEquipment) return { 0, false };

	auto pSrcStack = World.FindComponent<const CItemStackComponent>(StackID);
	if (!pSrcStack || !pSrcStack->Count) return { 0, false };

	if (Count > pSrcStack->Count) Count = pSrcStack->Count;
	U32 RemainingCount = Count;

	// First try to merge into existing stacks of the same item
	if (Merge)
	{
		// Check if the stack is moved within the same storage
		const auto IsAlreadyEquipped = IsStackEquipped(*pEquipment, StackID);

		for (auto [SlotID, SlotType] : pEquipment->Scheme->Slots)
		{
			auto It = pEquipment->Equipment.find(SlotID);
			if (It == pEquipment->Equipment.cend()) continue;

			auto pDestStack = World.FindComponent<const CItemStackComponent>(It->second);
			if (!pDestStack || !CanMergeStacks(*pSrcStack, pDestStack)) continue;

			const U32 SlotCapacity = CanEquipItems(World, EntityID, StackID, SlotID);
			if (pDestStack->Count >= SlotCapacity) continue;

			const auto [MovedCount, MovedCompletely] = MoveItemsToStack(World, It->second, StackID, std::min(RemainingCount, SlotCapacity - pDestStack->Count));
			if (MovedCompletely && IsAlreadyEquipped)
			{
				if (auto pEquipmentWritable = World.FindComponent<CEquipmentComponent>(EntityID))
				{
					const auto MainSlotID = UnblockEquipmentSlots(World, *pEquipmentWritable, StackID);
					RecordUnequipment(World, EntityID, StackID, EItemStorage::Equipment, MainSlotID);
				}
			}
			if (MovedCount >= RemainingCount) return { Count, MovedCompletely };
			RemainingCount -= MovedCount;
		}
	}

	// Put remaining count into free slots
	if (auto pEquipmentWritable = World.FindComponent<CEquipmentComponent>(EntityID))
	{
		for (auto [SlotID, SlotType] : pEquipment->Scheme->Slots)
		{
			auto& DestSlot = pEquipmentWritable->Equipment[SlotID];
			if (DestSlot) continue;

			const auto [MovedCount, MovedCompletely] = SplitItemsToSlot(World, DestSlot, StackID, std::min(RemainingCount, CanEquipItems(World, EntityID, StackID, SlotID)));
			if (MovedCount)
			{
				const auto MainSlotID = BlockEquipmentSlots(World, *pEquipmentWritable, DestSlot);
				RecordEquipment(World, EntityID, DestSlot, EItemStorage::Equipment, MainSlotID);
			}
			if (MovedCount >= RemainingCount) return { Count, MovedCompletely };
			RemainingCount -= MovedCount;
		}
	}

	return { Count - RemainingCount, false };
}
//---------------------------------------------------------------------

void ClearEquipmentSlot(Game::CGameWorld& World, Game::HEntity EntityID, CStrID SlotID)
{
	auto pEquipment = World.FindComponent<CEquipmentComponent>(EntityID);
	if (!pEquipment) return;
	auto It = pEquipment->Equipment.find(SlotID);
	if (It == pEquipment->Equipment.cend()) return;
	const auto StackID = It->second;
	const auto MainSlotID = UnblockEquipmentSlots(World, *pEquipment, StackID);
	RecordUnequipment(World, EntityID, StackID, EItemStorage::Equipment, MainSlotID);
}
//---------------------------------------------------------------------

// Returns a number of items actually removed
U32 RemoveItemsFromEquipmentSlot(Game::CGameWorld& World, Game::HEntity EntityID, CStrID SlotID, U32 Count)
{
	const auto [StackID, RemovedCount] = MoveItemsFromEquipmentSlot(World, EntityID, SlotID, Count);
	if (StackID && !RemovedCount) World.DeleteEntity(StackID);
	return RemovedCount;
}
//---------------------------------------------------------------------

//!!!FIXME: major code duplication with quick slots!
// Returns a number of items actually removed
U32 RemoveItemsFromEquipment(Game::CGameWorld& World, Game::HEntity EntityID, Game::HEntity ItemProtoID, U32 Count, bool AllowModified)
{
	if (!EntityID || !ItemProtoID || !Count) return 0;

	auto pEquipment = World.FindComponent<CEquipmentComponent>(EntityID);
	if (!pEquipment) return 0;

	auto pEquippable = FindItemComponent<const CEquippableComponent>(World, ItemProtoID);
	const auto MainSlotType = (pEquippable && !pEquippable->Slots.empty()) ? pEquippable->Slots.front().first : CStrID::Empty;

	// Delay slot unblocking to prevent iterator invalidation
	std::set<Game::HEntity> StacksToUnblock;
	CStrID MainSlotID;

	U32 RemainingCount = Count;
	for (auto It = pEquipment->Equipment.begin(); It != pEquipment->Equipment.end(); /**/)
	{
		const auto [SlotID, StackID] = *It;

		// Already counted and scheduled for unblocking
		// NB: we clear stack ID to optimize unblocking. One iteration of UnblockEquipmentSlots will be enough then.
		if (StacksToUnblock.find(StackID) != StacksToUnblock.cend())
		{
			It->second = {};
			continue;
		}

		auto pStack = World.FindComponent<const CItemStackComponent>(StackID);
		if (!pStack || pStack->Prototype != ItemProtoID || (!AllowModified && pStack->Modified)) continue;

		const auto MainSlotID = FindMainOccupiedSlot(World, *pEquipment, StackID, EItemStorage::Equipment).first;
		if (RemoveItemsFromStack(World, StackID, RemainingCount))
		{
			StacksToUnblock.insert(StackID);
			RecordUnequipment(World, EntityID, StackID, EItemStorage::Equipment, MainSlotID);
			It = pEquipment->Equipment.erase(It);
		}

		if (!RemainingCount) break;
	}

	// Unblock all slots occupied by removed stacks. We cleared them in advance to optimize an operation.
	UnblockEquipmentSlots(World, *pEquipment, {});

	return Count - RemainingCount;
}
//---------------------------------------------------------------------

U32 CanEquipItems(const Game::CGameWorld& World, Game::HEntity ReceiverID, Game::HEntity StackID, CStrID SlotID)
{
	auto pEquipment = World.FindComponent<const CEquipmentComponent>(ReceiverID);
	if (!pEquipment) return 0;

	auto pEquippable = FindItemComponent<const CEquippableComponent>(World, StackID);
	if (!pEquippable) return 0;

	// TODO: if scripted, try to find CanEquip function in the script
	// Return value will be true, false or nil. The latter is to proceed to the C++ logic below.

	// Check if requested destination slot exists and matches the required slot type of the equippable item
	auto ItSlotInfo = pEquipment->Scheme->Slots.find(SlotID);
	if (ItSlotInfo == pEquipment->Scheme->Slots.cend() || pEquippable->Slots.find(ItSlotInfo->second) == pEquippable->Slots.cend()) return 0;

	// The item in the destination slot can be replaced, so we consider slots blocked by it free
	const Game::HEntity ExcludedStackID = GetEquippedStack(*pEquipment, SlotID);

	// Find slots to equip the item into
	for (auto [RequiredSlotType, RequiredSlotCount] : pEquippable->Slots)
	{
		if (!RequiredSlotCount) continue;

		for (auto [CurrSlotID, CurrSlotType] : pEquipment->Scheme->Slots)
		{
			if (CurrSlotType != RequiredSlotType) continue;

			auto It = pEquipment->Equipment.find(CurrSlotID);
			if (It == pEquipment->Equipment.cend() || !It->second || It->second == ExcludedStackID)
				if (--RequiredSlotCount == 0)
					break;
		}

		// We failed to find enough free slots of this type
		if (RequiredSlotCount) return 0;
	}

	return std::max<U32>(1, pEquippable->MaxStack);
}
//---------------------------------------------------------------------

std::pair<CStrID, EItemStorage> FindMainOccupiedSlot(const Game::CGameWorld& World, const CEquipmentComponent& Equipment, Game::HEntity StackID, EItemStorage Storage)
{
	// Search in equipment
	if (Storage == EItemStorage::None || Storage == EItemStorage::Equipment)
	{
		auto pEquippable = FindItemComponent<const CEquippableComponent>(World, StackID);
		if (pEquippable && !pEquippable->Slots.empty())
		{
			const auto MainSlotType = pEquippable->Slots.front().first;
			for (const auto [SlotID, StackInSlotID] : Equipment.Equipment)
				if (StackID == StackInSlotID && MainSlotType == Equipment.Scheme->Slots[SlotID])
					return { SlotID, EItemStorage::Equipment };
		}
	}

	// Search in quick slots
	if (Storage == EItemStorage::None || Storage == EItemStorage::QuickSlot)
	{
		const auto FoundIndex = static_cast<size_t>(std::distance(Equipment.QuickSlots.cbegin(), std::find(Equipment.QuickSlots.cbegin(), Equipment.QuickSlots.cend(), StackID)));
		if (FoundIndex < Equipment.QuickSlots.size())
			return { GetQuickSlotID(FoundIndex), EItemStorage::QuickSlot };
	}

	return { CStrID::Empty, EItemStorage::None };
}
//---------------------------------------------------------------------

std::pair<CStrID, EItemStorage> FindMainOccupiedSlot(const Game::CGameWorld& World, Game::HEntity EntityID, Game::HEntity StackID, EItemStorage Storage)
{
	auto pEquipment = World.FindComponent<const CEquipmentComponent>(EntityID);
	if (!pEquipment) return { CStrID::Empty, EItemStorage::None };
	return FindMainOccupiedSlot(World, *pEquipment, StackID, Storage);
}
//---------------------------------------------------------------------

// Monitored and processed by the special system
void ScheduleStackReequipment(Game::CGameWorld& World, Game::HEntity StackID, EItemStorage Storage, CStrID SlotID)
{
	auto pEqupped = World.FindComponent<const CEquippedComponent>(StackID);
	if (!pEqupped) return;

	if (SlotID)
	{
		n_assert(Storage != EItemStorage::None);
	}
	else
	{
		// Find storage and slot
		std::tie(SlotID, Storage) = FindMainOccupiedSlot(World, pEqupped->OwnerID, StackID);
		if (!SlotID) return;
	}

	if (auto pChanges = World.FindOrAddComponent<CEquipmentChangesComponent>(pEqupped->OwnerID))
	{
		// Schedule re-equipment only if other changes for this stack are not scheduled
		auto It = pChanges->Records.find(StackID);
		if (It == pChanges->Records.cend())
			pChanges->Records.emplace(StackID, CEquipmentChangesComponent::CRecord{ SlotID, SlotID, Storage, Storage });
	}
}
//---------------------------------------------------------------------

void ScheduleReequipment(Game::CGameWorld& World, Game::HEntity ItemID)
{
	if (!ItemID) return;

	if (World.FindComponent<const CItemStackComponent>(ItemID))
	{
		ScheduleStackReequipment(World, ItemID);
	}
	else
	{
		World.ForEachComponent<const CItemStackComponent>([&World, ItemID](auto StackID, const CItemStackComponent& Stack)
		{
			if (Stack.Prototype == ItemID) ScheduleStackReequipment(World, StackID);
		});
	}
}
//---------------------------------------------------------------------

// Returns a number of items actually added
U32 AddItemsToLocation(Game::CGameWorld& World, Game::HEntity ItemProtoID, U32 Count, CStrID LevelID, const rtm::qvvf& Tfm, float MergeRadius)
{
	if (!LevelID || !ItemProtoID || !Count) return 0;

	auto pItem = World.FindComponent<const CItemComponent>(ItemProtoID);
	if (!pItem) return 0;

	// Try to merge into an existing stack or item pile container first
	if (MergeRadius > 0.f)
	{
		if (auto pLevel = World.FindLevel(LevelID))
		{
			// TODO:
			// Query item stacks and tmp item containers in accessible range, not owned by someone else (check ownership in CanMergeItems?)
			// If tmp item containers are found, add the stack to the closest one (take lookat dir into account?)
			// Else if stacks are found, try to merge into the closest one (take lookat dir into account?)
			// If not merged, create tmp item container and add found stack and our stack to it
			const auto DestStackID = FindClosestEntity(*pLevel, Tfm.translation, MergeRadius,
				[&World, ItemProtoID](Game::HEntity EntityID)
			{
				auto pGroundStack = World.FindComponent<const CItemStackComponent>(EntityID);
				return pGroundStack && CanMergeItems(ItemProtoID, pGroundStack);
			});

			if (auto pDestStack = World.FindComponent<CItemStackComponent>(DestStackID))
			{
				pDestStack->Count += Count;
				return Count;
			}
		}
	}

	//???TODO: rename to OnStackAddedToLocation or like that? The same name pattern for posteffects of every storage.
	return AddItemVisualsToLocation(World, CreateItemStack(World, ItemProtoID, Count, LevelID), Tfm) ? Count : 0;
}
//---------------------------------------------------------------------

// Returns a source stack ID if movement happened and a number of moved items (zero if the whole stack is moved)
std::pair<Game::HEntity, U32> MoveItemsFromLocation(Game::CGameWorld& World, Game::HEntity StackID, U32 Count)
{
	if (!StackID || !Count) return { {}, 0 };

	auto pStack = World.FindComponent<CItemStackComponent>(StackID);
	if (!pStack) return { {}, 0 };

	if (pStack->Count > Count)
	{
		pStack->Count -= Count;
		return { StackID, Count };
	}
	else
	{
		RemoveItemVisualsFromLocation(World, StackID);
		return { StackID, 0 };
	}
}
//---------------------------------------------------------------------

std::pair<U32, bool> MoveItemsToLocationSlot(Game::CGameWorld& World, std::vector<Game::HEntity>& GroundItems, size_t SlotIndex, Game::HEntity StackID,
	U32 Count, CStrID LevelID, const rtm::qvvf& Tfm, bool Merge, Game::HEntity* pReplaced)
{
	if (!StackID || !Count) return { 0, false };

	Game::HEntity DestStackID;
	if (SlotIndex < GroundItems.size())
	{
		DestStackID = GroundItems[SlotIndex];
		if (StackID == DestStackID) return { Count, false };
		if (!Merge && !*pReplaced && DestStackID) return { 0, false };
	}

	auto pSrcStack = World.FindComponent<const CItemStackComponent>(StackID);
	if (!pSrcStack || !pSrcStack->Count) return { 0, false };

	if (Count > pSrcStack->Count) Count = pSrcStack->Count;

	// Check if the stack is moved within the same storage
	const auto FoundIndex = static_cast<size_t>(std::distance(GroundItems.cbegin(), std::find(GroundItems.cbegin(), GroundItems.cend(), StackID)));
	const auto IsInternalMove = (FoundIndex < GroundItems.size());

	// Consider destination occupied only if the destination stack is valid
	if (auto pDestStack = World.FindComponent<const CItemStackComponent>(DestStackID))
	{
		if (Merge && CanMergeStacks(*pSrcStack, pDestStack))
		{
			const auto [MovedCount, MovedCompletely] = MoveItemsToStack(World, DestStackID, StackID, Count);
			if (MovedCompletely && IsInternalMove) ClearItemCollectionSlot(GroundItems, FoundIndex); // Don't remove world visuals
			return { MovedCount, MovedCompletely };
		}

		if (!pReplaced) return { 0, false };
	}

	if (SlotIndex >= GroundItems.size())
		GroundItems.resize(SlotIndex + 1);

	const auto [MovedCount, MovedCompletely] = SplitItemsToSlot(World, GroundItems[SlotIndex], StackID, Count);
	if (MovedCount)
	{
		// TODO: what to do with replaced stack visuals? How to handle moving of the item that already was on the ground? Needs testing!
		if (!AddItemVisualsToLocation(World, GroundItems[SlotIndex], Tfm)) return { 0, false };
		if (pReplaced) *pReplaced = DestStackID;
		if (MovedCompletely && IsInternalMove) ClearItemCollectionSlot(GroundItems, FoundIndex); // Don't remove world visuals
		return { MovedCount, MovedCompletely };
	}

	return { 0, false };
}
//---------------------------------------------------------------------

// Returns a number of items actually moved in and a 'need to clear source storage' flag
std::pair<U32, bool> MoveItemsToLocation(Game::CGameWorld& World, Game::HEntity StackID, U32 Count, CStrID LevelID, const rtm::qvvf& Tfm, float MergeRadius)
{
	if (!LevelID || !StackID || !Count) return { 0, false };

	auto pSrcStack = World.FindComponent<const CItemStackComponent>(StackID);
	if (!pSrcStack || !pSrcStack->Count) return { 0, false };

	if (Count > pSrcStack->Count) Count = pSrcStack->Count;

	// Try to merge into an existing stack or item pile container first
	if (MergeRadius > 0.f)
	{
		if (auto pLevel = World.FindLevel(LevelID))
		{
			// TODO:
			// Query item stacks and tmp item containers in accessible range, not owned by someone else (check ownership in CanMergeItems?)
			// If tmp item containers are found, add the stack to the closest one (take lookat dir into account?)
			// Else if stacks are found, try to merge into the closest one (take lookat dir into account?)
			// If not merged, create tmp item container and add found stack and our stack to it
			const auto DestStackID = FindClosestEntity(*pLevel, Tfm.translation, MergeRadius,
				[&World, pSrcStack](Game::HEntity EntityID)
			{
				auto pGroundStack = World.FindComponent<const CItemStackComponent>(EntityID);
				return pGroundStack && CanMergeStacks(*pSrcStack, pGroundStack);
			});

			return MoveItemsToStack(World, DestStackID, StackID, Count);
		}
	}

	// Allocate in the world
	//!!!TODO: SplitItemStack - ensure that the stack was created in the correct game level!
	//???TODO: rename AddItemVisualsToLocation to OnStackAddedToLocation or like that? The same name pattern for posteffects of every storage.
	const auto NewStackID = SplitItemStack(World, StackID, Count);
	if (!AddItemVisualsToLocation(World, NewStackID, Tfm)) return { 0, false };
	return { Count, NewStackID == StackID };
}
//---------------------------------------------------------------------

Game::HEntity MoveWholeStackToLocation(Game::CGameWorld& World, Game::HEntity StackID, CStrID LevelID, const rtm::qvvf& Tfm, float MergeRadius)
{
	if (!LevelID || !StackID) return {};

	auto pSrcStack = World.FindComponent<const CItemStackComponent>(StackID);
	if (!pSrcStack || !pSrcStack->Count) return {};

	// Try to merge into an existing stack or item pile container first
	if (MergeRadius > 0.f)
	{
		if (auto pLevel = World.FindLevel(LevelID))
		{
			// TODO: see the comment for the same call inside MoveItemsToLocation
			const auto DestStackID = FindClosestEntity(*pLevel, Tfm.translation, MergeRadius,
				[&World, pSrcStack](Game::HEntity EntityID)
			{
				auto pGroundStack = World.FindComponent<const CItemStackComponent>(EntityID);
				return pGroundStack && CanMergeStacks(*pSrcStack, pGroundStack);
			});

			return MoveItemsToStack(World, DestStackID, StackID, pSrcStack->Count).first ? DestStackID : Game::HEntity{};
		}
	}

	// Allocate in the world
	return AddItemVisualsToLocation(World, StackID, Tfm) ? StackID : Game::HEntity{};
}
//---------------------------------------------------------------------

void ClearLocationSlot(Game::CGameWorld& World, std::vector<Game::HEntity>& GroundItems, size_t SlotIndex)
{
	if (SlotIndex >= GroundItems.size()) return;
	RemoveItemVisualsFromLocation(World, GroundItems[SlotIndex]);
	ClearItemCollectionSlot(GroundItems, SlotIndex);
}
//---------------------------------------------------------------------

// Returns a number of items actually removed
U32 RemoveItemsFromLocation(Game::CGameWorld& World, Game::HEntity StackID, U32 Count)
{
	const auto [RemovedStackID, RemovedCount] = MoveItemsFromLocation(World, StackID, Count);
	if (RemovedStackID && !RemovedCount) World.DeleteEntity(RemovedStackID);
	return RemovedCount;
}
//---------------------------------------------------------------------

bool AddItemVisualsToLocation(Game::CGameWorld& World, Game::HEntity StackID, const rtm::qvvf& Tfm)
{
	auto pItemStack = World.FindComponent<const CItemStackComponent>(StackID);
	if (!pItemStack) return false;

	const CItemComponent* pItem = FindItemComponent<const CItemComponent>(World, StackID, *pItemStack);
	if (!pItem) return false;

	if (pItem->WorldModelID)
	{
		auto pSceneComponent = World.AddComponent<Game::CSceneComponent>(StackID);
		pSceneComponent->RootNode->RemoveFromParent();
		pSceneComponent->AssetID = pItem->WorldModelID;
		pSceneComponent->RootNode->SetLocalTransform(Tfm);
	}

	if (pItem->WorldPhysicsID)
	{
		auto pPhysicsComponent = World.AddComponent<Game::CRigidBodyComponent>(StackID);
		pPhysicsComponent->ShapeAssetID = pItem->WorldPhysicsID;
		pPhysicsComponent->Mass = pItemStack->Count * pItem->Weight;
		pPhysicsComponent->CollisionGroupID = CStrID("PhysicalDynamic|Interactable");
		// TODO: physics material?
	}

	return true;
}
//---------------------------------------------------------------------

void RemoveItemVisualsFromLocation(Game::CGameWorld& World, Game::HEntity StackID)
{
	World.RemoveComponent<Game::CSceneComponent>(StackID);
	World.RemoveComponent<Game::CRigidBodyComponent>(StackID);
}
//---------------------------------------------------------------------

}
