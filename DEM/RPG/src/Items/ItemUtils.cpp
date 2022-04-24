#include "ItemUtils.h"
#include <Items/ItemComponent.h>
#include <Items/ItemContainerComponent.h>
#include <Items/EquipmentComponent.h>
#include <Items/EquippableComponent.h>
#include <Scene/SceneComponent.h>
#include <Physics/RigidBodyComponent.h>

namespace DEM::RPG
{

//!!!FIXME: where to place?! Next to EEquipmentSlot enum?
constexpr EEquipmentSlotType EEquipmentSlot_Type[] =
{
	EEquipmentSlotType::Torso,
	EEquipmentSlotType::Shoulders,
	EEquipmentSlotType::Head,
	EEquipmentSlotType::Arms,
	EEquipmentSlotType::Hands,
	EEquipmentSlotType::Legs,
	EEquipmentSlotType::Feet,
	EEquipmentSlotType::Belt,
	EEquipmentSlotType::Backpack,
	EEquipmentSlotType::Neck,
	EEquipmentSlotType::Bracelet,
	EEquipmentSlotType::Bracelet,
	EEquipmentSlotType::Ring,
	EEquipmentSlotType::Ring,
	EEquipmentSlotType::Ring,
	EEquipmentSlotType::Ring,
	EEquipmentSlotType::HandItem,
	EEquipmentSlotType::HandItem,
	EEquipmentSlotType::HandItem,
	EEquipmentSlotType::HandItem
};

// Returns a number of items actually added
U32 AddItemsToEntity(Game::CGameWorld& World, Game::HEntity ProtoID, U32 Count, Game::HEntity ReceiverID, EItemStorage Storage, size_t Index, bool Merge)
{
	if (!ReceiverID || !ProtoID || !Count || Storage == EItemStorage::None || Storage == EItemStorage::World) return 0;

	//???TODO: make internal version where this is passed from outside? to optimize frequent access.
	auto pItem = World.FindComponent<const CItemComponent>(ProtoID);
	if (!pItem) return 0;

	const Game::HEntity* pSlotConst = GetItemSlot(World, ReceiverID, Storage, Index);
	if (!pSlotConst) return 0;

	const auto DestStackID = *pSlotConst;

	// Can't add to slot occupied by an incompatible item stack
	// NB: dest stack is accessed for reading
	U32 CountInSlot = 0;
	if (auto pDestStack = World.FindComponent<const CItemStackComponent>(DestStackID))
	{
		if (!CanMergeItems(ProtoID, pDestStack)) return 0;
		CountInSlot = pDestStack->Count;
	}

	U32 MaxCountInSlot = 0;
	if (pItem->Volume <= 0.f)
	{
		MaxCountInSlot = std::numeric_limits<U32>().max();
	}
	else
	{
		switch (Storage)
		{
			case EItemStorage::Container:
			{
				//???TODO: make internal version where this is passed from outside? to optimize frequent access.
				auto pInventory = World.FindComponent<const CItemContainerComponent>(ReceiverID);
				if (!pInventory) return 0;

				CContainerStats Stats;
				CalcContainerStats(World, *pInventory, Stats);
				MaxCountInSlot = Stats.FreeVolume / pItem->Volume + CountInSlot;
				break;
			}
			case EItemStorage::QuickSlot:
			{
				auto pEquipment = World.FindComponent<const Sh2::CEquipmentComponent>(ReceiverID);
				if (!pEquipment || Index >= pEquipment->QuickSlots.size()) return 0;
				MaxCountInSlot = QUICK_SLOT_VOLUME / pItem->Volume;
				break;
			}
			case EItemStorage::Equipment:
			{
				if (Index >= Sh2::EEquipmentSlot::COUNT) return 0;
				MaxCountInSlot = CanEquipItems(World, ReceiverID, ProtoID, EEquipmentSlot_Type[Index]);
				break;
			}
			case EItemStorage::World:
			{
				MaxCountInSlot = std::numeric_limits<U32>().max();
				break;
			}
		}
	}

	const U32 AddCount = std::min(Count, MaxCountInSlot - CountInSlot);
	if (!AddCount) return 0;

	// Now access destination for writing
	if (auto pDestStack = World.FindComponent<CItemStackComponent>(DestStackID))
	{
		pDestStack->Count += AddCount;
	}
	else if (auto pSlot = GetItemSlotWritable(World, ReceiverID, Storage, Index))
	{
		CStrID LevelID; //???!!!need? can copy from Receiver or leave empty!

		//???!!!pass CStrID ItemID or HEntity ProtoID?!
		//*pSlot = CItemManager::CreateStack(ProtoID, AddCount, LevelID);

		// FIXME: duplication, see CItemManager::CreateStack!
		Game::HEntity StackID = World.CreateEntity(LevelID);
		auto pStack = World.AddComponent<CItemStackComponent>(StackID);
		if (!pStack)
		{
			World.DeleteEntity(StackID);
			return 0;
		}

		pStack->Prototype = ProtoID;
		pStack->Count = AddCount;

		*pSlot = StackID;
	}

	return AddCount;
}
//---------------------------------------------------------------------

// TODO: optional merge radius? < 0.f means no merge allowed
void AddItemsToWorld(Game::CGameWorld& World, Game::HEntity ProtoID, U32 Count, Math::CTransform Tfm)
{
	// merge if allowed and possible
	// create new stack
	// init world components for it and position where requested
}
//---------------------------------------------------------------------

static U32 GetContainerCapacityInItems(const Game::CGameWorld& World, const CItemContainerComponent& Container,
	const CItemComponent* pItem, U32 MinItemCount, Game::HEntity ExcludeStackID)
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

	return static_cast<U32>(FreeVolume / pItem->Volume);
}
//---------------------------------------------------------------------

// Returns not transferred count
U32 AddItemsIntoContainer(Game::CGameWorld& World, Game::HEntity Receiver, Game::HEntity StackID, bool Merge, bool Split)
{
	auto pStack = World.FindComponent<CItemStackComponent>(StackID);
	n_assert_dbg(pStack && pStack->Count);
	if (!pStack || !pStack->Count) return 0;

	auto pContainer = World.FindComponent<CItemContainerComponent>(Receiver);
	if (!pContainer) return pStack->Count;

	// Check that we don't insert already contained stack
	if (std::find(pContainer->Items.cbegin(), pContainer->Items.cend(), StackID) != pContainer->Items.cend())
		return 0;

	// Check volume limits. Weight doesn't block adding items, only carrying them.
	auto TransferCount = pStack->Count;
	if (pContainer->MaxVolume >= 0.f)
	{
		auto pItem = FindItemComponent<const CItemComponent>(World, StackID, *pStack);
		const auto Capacity = GetContainerCapacityInItems(World, *pContainer, pItem, Split ? 1 : pStack->Count, {});
		if (!Capacity) return pStack->Count;
		TransferCount = std::min(TransferCount, Capacity);
	}

	// Items inside a container are always hidden from the view
	const U32 RemainingCount = pStack->Count - TransferCount;
	if (!RemainingCount)
		RemoveItemsFromLocation(World, StackID);

	// Try to merge new items into existing stack
	// TODO: maybe will need to remove "!pStack->Modified" in the future. See comments inside CanMergeStacks()!
	if (Merge && !pStack->Modified)
	{
		for (auto MergeAcceptorID : pContainer->Items)
		{
			auto pMergeTo = World.FindComponent<CItemStackComponent>(MergeAcceptorID);
			if (CanMergeStacks(*pStack, pMergeTo))
			{
				pMergeTo->Count += TransferCount;
				if (RemainingCount)
					pStack->Count = RemainingCount;
				else
					World.DeleteEntity(StackID);
				return RemainingCount;
			}
		}
	}

	// Split the stack if needed
	auto NewStackID = StackID;
	if (RemainingCount)
	{
		pStack->Count = RemainingCount;
		NewStackID = World.CloneEntityExcluding<Game::CSceneComponent, Game::CRigidBodyComponent>(StackID);
		if (auto pNewStack = World.FindComponent<CItemStackComponent>(NewStackID))
			pNewStack->Count = TransferCount;
	}

	// Put the stack into the container
	auto It = std::find(pContainer->Items.begin(), pContainer->Items.end(), DEM::Game::HEntity{});
	if (It == pContainer->Items.cend())
		pContainer->Items.push_back(NewStackID);
	else
		(*It) = NewStackID;

	return RemainingCount;
}
//---------------------------------------------------------------------

// Returns not transferred count
U32 AddItemsIntoQuickSlots(Game::CGameWorld& World, Game::HEntity Receiver, Game::HEntity StackID, bool Merge, bool Split)
{
	auto pStack = World.FindComponent<CItemStackComponent>(StackID);
	n_assert_dbg(pStack && pStack->Count);
	if (!pStack || !pStack->Count) return 0;

	auto pEquipment = World.FindComponent<Sh2::CEquipmentComponent>(Receiver);
	if (!pEquipment) return pStack->Count;

	// Check that we don't insert already contained stack
	if (std::find(pEquipment->QuickSlots.cbegin(), pEquipment->QuickSlots.cend(), StackID) != pEquipment->QuickSlots.cend())
		return 0;

	auto pItem = FindItemComponent<const CItemComponent>(World, StackID, *pStack);
	const U32 ItemsPerSlot = (pItem && pItem->Volume > 0.f) ?
		static_cast<U32>(QUICK_SLOT_VOLUME / pItem->Volume) :
		std::numeric_limits<U32>().max();

	if (ItemsPerSlot < pStack->Count && !Split) return pStack->Count;

	// First try to merge into existing stacks of the same item
	if (Merge)
	{
		for (auto MergeAcceptorID : pEquipment->QuickSlots)
		{
			auto pMergeTo = World.FindComponent<CItemStackComponent>(MergeAcceptorID);

			// Skip immediately if count limit is reached, no matter what item is there
			if (!pMergeTo || pMergeTo->Count >= ItemsPerSlot) continue;

			if (CanMergeStacks(*pStack, pMergeTo))
			{
				const U32 RemainingCount = ItemsPerSlot - pMergeTo->Count;
				if (pStack->Count <= RemainingCount)
				{
					pMergeTo->Count += pStack->Count;
					World.DeleteEntity(StackID);
					return 0;
				}
				else if (Split)
				{
					pStack->Count -= RemainingCount;
					pMergeTo->Count += RemainingCount;
				}
			}
		}
	}

	// Put not merged items into free slots
	for (auto& DestSlot : pEquipment->QuickSlots)
	{
		if (DestSlot) continue;

		if (pStack->Count <= ItemsPerSlot)
		{
			DestSlot = StackID;
			return 0;
		}
		else
		{
			pStack->Count -= ItemsPerSlot;
			const auto NewStackID =
				World.CloneEntityExcluding<Game::CSceneComponent, Game::CRigidBodyComponent>(StackID);
			if (auto pNewStack = World.FindComponent<CItemStackComponent>(NewStackID))
				pNewStack->Count = ItemsPerSlot;
			DestSlot = NewStackID;
		}
	}

	return pStack->Count;
}
//---------------------------------------------------------------------

// Returns not transferred count
static U32 StoreItemStack(Game::CGameWorld& World, Game::HEntity Receiver, EItemStorage Storage, Game::HEntity StackID, bool Merge, bool Split)
{
	switch (Storage)
	{
		case EItemStorage::Container:
		{
			return AddItemsIntoContainer(World, Receiver, StackID, Merge, Split);
		}
		case EItemStorage::QuickSlot:
		{
			return AddItemsIntoQuickSlots(World, Receiver, StackID, Merge, Split);
		}
		case EItemStorage::Equipment:
		{
			// limit - single item. Equipment is not a general purpose slot, these are QuickSlots.
			break;
		}
		case EItemStorage::World:
		{
			NOT_IMPLEMENTED;

			/*
			//!!!ground merging, tmp containers etc!
			//!!!update inventory screen ground list if opened! Common logic must send a signal?

			//!!!DUPLICATED CODE, SEE OnSlotDropped! Must be in a single place where tmp container creation etc is handled!
			Math::CTransformSRT Tfm;
			if (auto pOwnerScene = World.FindComponent<const DEM::Game::CSceneComponent>(Owner))
				Tfm.Translation = pOwnerScene->RootNode->GetWorldMatrix().transform_coord(-vector3::AxisZ);
			DropItemsToLocation(World, StackID, Tfm);
			*/
			break;
		}
	}

	auto pStack = World.FindComponent<const CItemStackComponent>(StackID);
	return pStack ? pStack->Count : 0;
}
//---------------------------------------------------------------------

//!!!TODO: use std::span for the storage order!
// Returns not transferred count
static U32 StoreItemStack(Game::CGameWorld& World, Game::HEntity Receiver, const EItemStorage* pStorageOrder, UPTR StorageCount, Game::HEntity StackID, bool Merge, bool Split)
{
	auto pStack = World.FindComponent<const CItemStackComponent>(StackID);
	if (!pStack || !pStack->Count) return 0;

	if (pStorageOrder)
		for (UPTR i = 0; i < StorageCount; ++i)
			if (!StoreItemStack(World, Receiver, pStorageOrder[i], StackID, Merge, Split))
				return 0;

	return pStack->Count;
}
//---------------------------------------------------------------------

// Returns not transferred count
U32 AddItemsToCharacter(Game::CGameWorld& World, Game::HEntity Receiver, Game::HEntity StackID, EItemStorage PreferredStorage, bool AllowGround, bool Merge, bool Split)
{
	const EItemStorage* pStorageOrder = nullptr;
	UPTR StorageCount = 0;

	constexpr EItemStorage StorageOrderEquipment[] = { EItemStorage::Equipment, EItemStorage::Container, EItemStorage::QuickSlot };
	constexpr EItemStorage StorageOrderInventory[] = { EItemStorage::Container, EItemStorage::QuickSlot };
	constexpr EItemStorage StorageOrderQuickSlot[] = { EItemStorage::QuickSlot, EItemStorage::Container };
	switch (PreferredStorage)
	{
		case EItemStorage::Equipment: pStorageOrder = StorageOrderEquipment; StorageCount = sizeof_array(StorageOrderEquipment); break;
		case EItemStorage::Container: pStorageOrder = StorageOrderInventory; StorageCount = sizeof_array(StorageOrderInventory); break;
		case EItemStorage::QuickSlot: pStorageOrder = StorageOrderQuickSlot; StorageCount = sizeof_array(StorageOrderQuickSlot); break;
		case EItemStorage::World: break;
		default: return false;
	}

	if (!StoreItemStack(World, Receiver, pStorageOrder, StorageCount, StackID, Merge, Split)) return 0;

	if (AllowGround)
		return StoreItemStack(World, Receiver, EItemStorage::World, StackID, Merge, Split);

	auto pStack = World.FindComponent<const CItemStackComponent>(StackID);
	return pStack ? pStack->Count : 0;
}
//---------------------------------------------------------------------

Game::HEntity AddStackIntoCollection(Game::CGameWorld& World, std::vector<Game::HEntity>& Collection, Game::HEntity StackID, bool Merge)
{
	// Try to merge new items into existing stack
	if (Merge)
	{
		// TODO: maybe will need to remove !Modified condition in the future. See CanMergeStacks() logic!
		//???don't merge items with different owners?!
		auto pStack = World.FindComponent<const CItemStackComponent>(StackID);
		if (pStack && !pStack->Modified)
		{
			for (auto MergeAcceptorID : Collection)
			{
				auto pMergeTo = World.FindComponent<CItemStackComponent>(MergeAcceptorID);
				if (CanMergeStacks(*pStack, pMergeTo))
				{
					pMergeTo->Count += pStack->Count;
					World.DeleteEntity(StackID);
					return MergeAcceptorID;
				}
			}
		}
	}

	// If not merged, transfer a stack into the container
	auto It = std::find(Collection.begin(), Collection.end(), DEM::Game::HEntity{});
	if (It == Collection.cend())
		Collection.push_back(StackID);
	else
		(*It) = StackID;

	return StackID;
}
//---------------------------------------------------------------------

void ShrinkItemCollection(std::vector<Game::HEntity>& Collection)
{
	// Trim the tail to the last busy slot
	auto RIt = ++Collection.rbegin();
	for (; RIt != Collection.rend(); ++RIt)
		if (*RIt) break;
	Collection.erase(RIt.base(), Collection.end());
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

void UpdateCharacterModelEquipment(Game::CGameWorld& World, Game::HEntity OwnerID, Sh2::EEquipmentSlot Slot, bool ForceHide)
{
	if (Slot >= Sh2::EEquipmentSlot::COUNT) return;

	//!!!FIXME: where to place?!
	constexpr const char* EEquipmentSlot_Bone[] =
	{
		"body",
		"shoulders_cloak",
		"head",
		"arms",
		"hands",
		"legs",
		"feet",
		"belt",
		"backpack",
		"neck",
		"bracelet",
		"bracelet",
		"ring",
		"ring",
		"ring",
		"ring",
		"mixamorig_RightHandMiddle1",
		"mixamorig_LeftHandMiddle1",
		"mixamorig_RightHandMiddle1",
		"mixamorig_LeftHandMiddle1"
	};

	//!!!FIXME: constexpr CStrID?! Or at least pre-init once!
	const auto pBoneName = EEquipmentSlot_Bone[Slot];
	if (!pBoneName || !*pBoneName) return;

	auto pEquipment = World.FindComponent<const Sh2::CEquipmentComponent>(OwnerID);
	if (!pEquipment) return;

	const auto StackID = pEquipment->Equipment[Slot];

	auto pOwnerScene = World.FindComponent<const DEM::Game::CSceneComponent>(OwnerID);
	if (!pOwnerScene || !pOwnerScene->RootNode) return;

	auto pBone = pOwnerScene->RootNode->GetChildRecursively(CStrID(pBoneName));
	if (!pBone) return;

	if (StackID && !ForceHide)
	{
		const CItemComponent* pItem = FindItemComponent<const CItemComponent>(World, StackID);
		if (!pItem || !pItem->WorldModelID) return;

		if (pBone->IsWorldTransformDirty()) pBone->UpdateTransform();

		// Undo scaling
		Math::CTransformSRT Tfm(pBone->GetWorldMatrix());
		Tfm.Scale.ReciprocalInplace();
		Tfm.Rotation = quaternion::Identity;
		Tfm.Translation = vector3::Zero;

		//!!!TODO: store desired attachment local rotation and translation (and scaling?) somewhere?

		//!!!DBG TMP!
		Tfm.Rotation.set_rotate_x(HALF_PI);

		auto pSceneComponent = World.AddComponent<Game::CSceneComponent>(StackID);
		pBone->AddChild(CStrID("Equipment"), pSceneComponent->RootNode, true);
		pSceneComponent->AssetID = pItem->WorldModelID;
		pSceneComponent->SetLocalTransform(Tfm);
	}
	else
	{
		pBone->RemoveChild(CStrID("Equipment"));
	}
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

Game::HEntity* GetItemSlotWritable(Game::CGameWorld& World, Game::HEntity ReceiverID, EItemStorage Storage, UPTR Index)
{
	switch (Storage)
	{
		case EItemStorage::Container:
		{
			auto pInventory = World.FindComponent<CItemContainerComponent>(ReceiverID);
			return (pInventory && Index < pInventory->Items.size()) ? &pInventory->Items[Index] : nullptr;
		}
		case EItemStorage::QuickSlot:
		{
			auto pEquipment = World.FindComponent<Sh2::CEquipmentComponent>(ReceiverID);
			return (pEquipment && Index < pEquipment->QuickSlots.size()) ? &pEquipment->QuickSlots[Index] : nullptr;
		}
		case EItemStorage::Equipment:
		{
			if (Index >= Sh2::EEquipmentSlot::COUNT) return nullptr;
			auto pEquipment = World.FindComponent<Sh2::CEquipmentComponent>(ReceiverID);
			return (pEquipment && (pEquipment->SlotEnabledBits & (1 << Index))) ? &pEquipment->Equipment[Index] : nullptr;
		}
	}

	return nullptr;
}
//---------------------------------------------------------------------

const Game::HEntity* GetItemSlot(Game::CGameWorld& World, Game::HEntity ReceiverID, EItemStorage Storage, UPTR Index)
{
	switch (Storage)
	{
		case EItemStorage::Container:
		{
			auto pInventory = World.FindComponent<const CItemContainerComponent>(ReceiverID);
			return (pInventory && Index < pInventory->Items.size()) ? &pInventory->Items[Index] : nullptr;
		}
		case EItemStorage::QuickSlot:
		{
			auto pEquipment = World.FindComponent<const Sh2::CEquipmentComponent>(ReceiverID);
			return (pEquipment && Index < pEquipment->QuickSlots.size()) ? &pEquipment->QuickSlots[Index] : nullptr;
		}
		case EItemStorage::Equipment:
		{
			if (Index >= Sh2::EEquipmentSlot::COUNT) return nullptr;
			auto pEquipment = World.FindComponent<const Sh2::CEquipmentComponent>(ReceiverID);
			return (pEquipment && (pEquipment->SlotEnabledBits & (1 << Index))) ? &pEquipment->Equipment[Index] : nullptr;
		}
	}

	return nullptr;
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

U32 CalcCapacityForItemProto(Game::CGameWorld& World, Game::HEntity ProtoID, Game::HEntity ReceiverID, EItemStorage Storage, UPTR Index)
{
	switch (Storage)
	{
		case EItemStorage::Container:
		{
			auto pInventory = World.FindComponent<const CItemContainerComponent>(ReceiverID);
			if (!pInventory) return 0;

			if (pInventory->MaxVolume < 0.f) return std::numeric_limits<U32>().max();

			if (std::find(pInventory->Items.cbegin(), pInventory->Items.cend(), StackID) != pInventory->Items.cend())
				return std::numeric_limits<U32>().max();

			auto pStack = World.FindComponent<const CItemStackComponent>(StackID);
			if (!pStack || !pStack->Count) return 0;

			// We will not replace the stack if we merge with it
			auto ReplacedStackID = (Index < pInventory->Items.size()) ? pInventory->Items[Index] : Game::HEntity{};
			auto pReplacedStack = World.FindComponent<const CItemStackComponent>(ReplacedStackID);
			if (CanMergeItems(*pStack, pReplacedStack))
				ReplacedStackID = {};

			auto pItem = FindItemComponent<const CItemComponent>(World, StackID, *pStack);
			return GetContainerCapacityInItems(World, *pInventory, pItem, 1, ReplacedStackID);
		}
		case EItemStorage::QuickSlot:
		{
			auto pEquipment = World.FindComponent<const Sh2::CEquipmentComponent>(ReceiverID);
			if (!pEquipment || Index >= pEquipment->QuickSlots.size()) return 0;

			auto ReplacedStackID = pEquipment->QuickSlots[Index];
			if (StackID == ReplacedStackID) return std::numeric_limits<U32>().max();

			auto pStack = World.FindComponent<const CItemStackComponent>(StackID);
			if (!pStack || !pStack->Count) return 0;

			auto pItem = FindItemComponent<const CItemComponent>(World, StackID, *pStack);
			if (!pItem || pItem->Volume <= 0.f) return std::numeric_limits<U32>().max();

			float FreeVolume = QUICK_SLOT_VOLUME;

			// Subtract volume of the element to be merged with
			auto pReplacedStack = World.FindComponent<const CItemStackComponent>(ReplacedStackID);
			if (CanMergeStacks(*pStack, pReplacedStack))
				FreeVolume -= (pReplacedStack->Count * pItem->Volume);

			return static_cast<U32>(FreeVolume / pItem->Volume);
		}
		case EItemStorage::Equipment:
		{
			if (Index >= Sh2::EEquipmentSlot::COUNT) return 0;

			auto pEquipment = World.FindComponent<const Sh2::CEquipmentComponent>(ReceiverID);
			if (!pEquipment || !(pEquipment->SlotEnabledBits & (1 << Index))) return 0;

			auto ReplacedStackID = pEquipment->Equipment[Index];
			if (StackID == ReplacedStackID) return std::numeric_limits<U32>().max();

			auto pStack = World.FindComponent<const CItemStackComponent>(StackID);
			if (!pStack || !pStack->Count) return 0;

			// Merging equipment is forbidden
			auto pReplacedStack = World.FindComponent<const CItemStackComponent>(ReplacedStackID);
			if (CanMergeStacks(*pStack, pReplacedStack)) return 0;

			return CanEquipItems(World, ReceiverID, StackID, EEquipmentSlot_Type[Index]);
		}
		case EItemStorage::World:
		{
			return std::numeric_limits<U32>().max();
		}
	}

	return 0;
}
//---------------------------------------------------------------------

U32 CalcCapacityForItemStack(Game::CGameWorld& World, Game::HEntity StackID, Game::HEntity ReceiverID, EItemStorage Storage, UPTR Index)
{
	const Game::HEntity* pSlot = GetItemSlot(World, ReceiverID, Storage, Index);
	if (pSlot && *pSlot == StackID) return std::numeric_limits<U32>().max();

	// get DestStackID from Receiver, DestStorage and DestIndex
	// if the same as src ID, ret max

	switch (DestStorage)
	{
		case EItemStorage::Container:
		{
			auto pInventory = World.FindComponent<const CItemContainerComponent>(Receiver);
			if (!pInventory) return 0;

			if (pInventory->MaxVolume < 0.f) return std::numeric_limits<U32>().max();

			if (std::find(pInventory->Items.cbegin(), pInventory->Items.cend(), StackID) != pInventory->Items.cend())
				return std::numeric_limits<U32>().max();

			auto pStack = World.FindComponent<const CItemStackComponent>(StackID);
			if (!pStack || !pStack->Count) return 0;

			// We will not replace the stack if we merge with it
			auto ReplacedStackID = (DestIndex < pInventory->Items.size()) ? pInventory->Items[DestIndex] : Game::HEntity{};
			auto pReplacedStack = World.FindComponent<const CItemStackComponent>(ReplacedStackID);
			if (CanMergeStacks(*pStack, pReplacedStack))
				ReplacedStackID = {};

			auto pItem = FindItemComponent<const CItemComponent>(World, StackID, *pStack);
			return GetContainerCapacityInItems(World, *pInventory, pItem, 1, ReplacedStackID);
		}
		case EItemStorage::QuickSlot:
		{
			auto pEquipment = World.FindComponent<const Sh2::CEquipmentComponent>(Receiver);
			if (!pEquipment || DestIndex >= pEquipment->QuickSlots.size()) return 0;

			auto ReplacedStackID = pEquipment->QuickSlots[DestIndex];
			if (StackID == ReplacedStackID) return std::numeric_limits<U32>().max();

			auto pStack = World.FindComponent<const CItemStackComponent>(StackID);
			if (!pStack || !pStack->Count) return 0;

			auto pItem = FindItemComponent<const CItemComponent>(World, StackID, *pStack);
			if (!pItem || pItem->Volume <= 0.f) return std::numeric_limits<U32>().max();

			float FreeVolume = QUICK_SLOT_VOLUME;

			// Subtract volume of the element to be merged with
			auto pReplacedStack = World.FindComponent<const CItemStackComponent>(ReplacedStackID);
			if (CanMergeStacks(*pStack, pReplacedStack))
				FreeVolume -= (pReplacedStack->Count * pItem->Volume);

			return static_cast<U32>(FreeVolume / pItem->Volume);
		}
		case EItemStorage::Equipment:
		{
			if (DestIndex >= Sh2::EEquipmentSlot::COUNT) return 0;

			auto pEquipment = World.FindComponent<const Sh2::CEquipmentComponent>(Receiver);
			if (!pEquipment || !(pEquipment->SlotEnabledBits & (1 << DestIndex))) return 0;

			auto ReplacedStackID = pEquipment->Equipment[DestIndex];
			if (StackID == ReplacedStackID) return std::numeric_limits<U32>().max();

			auto pStack = World.FindComponent<const CItemStackComponent>(StackID);
			if (!pStack || !pStack->Count) return 0;

			// Merging equipment is forbidden
			auto pReplacedStack = World.FindComponent<const CItemStackComponent>(ReplacedStackID);
			if (CanMergeStacks(*pStack, pReplacedStack)) return 0;

			return CanEquipItems(World, Receiver, StackID, EEquipmentSlot_Type[DestIndex]);
		}
		case EItemStorage::World:
		{
			return std::numeric_limits<U32>().max();
		}
	}

	return 0;
}
//---------------------------------------------------------------------

Game::HEntity TransferItems(Game::CGameWorld& World, U32 Count, Game::HEntity& SrcSlot, Game::HEntity& DestSlot)
{
	const auto SrcStackID = SrcSlot;
	const auto DestStackID = DestSlot;

	if (!Count || !SrcStackID || SrcStackID == DestStackID) return SrcStackID;

	auto pSrcStack = World.FindComponent<CItemStackComponent>(SrcStackID);
	if (!pSrcStack) return {};

	const bool TransferWholeStack = (pSrcStack->Count == Count);
	if (TransferWholeStack)
		SrcSlot = {};
	else
		pSrcStack->Count -= Count;

	auto pDestStack = World.FindComponent<CItemStackComponent>(DestStackID);
	if (CanMergeStacks(*pSrcStack, pDestStack))
	{
		pDestStack->Count += Count;
		if (TransferWholeStack) World.DeleteEntity(SrcStackID);
	}
	else
	{
		if (TransferWholeStack)
		{
			DestSlot = SrcStackID;
		}
		else
		{
			const auto NewStackID =
				World.CloneEntityExcluding<Game::CSceneComponent, Game::CRigidBodyComponent>(SrcStackID);
			if (auto pNewStack = World.FindComponent<CItemStackComponent>(NewStackID))
				pNewStack->Count = Count;
			DestSlot = NewStackID;
		}

		// If we replaced some stack, start dragging it instead of the source stack
		if (DestStackID) return DestStackID;
	}

	return SrcStackID;
}
//---------------------------------------------------------------------

U32 CanEquipItems(const Game::CGameWorld& World, Game::HEntity Receiver, Game::HEntity StackID, EEquipmentSlotType SlotType)
{
	U32 MaxStack = 1;

	if (auto pEquippable = FindItemComponent<const CEquippableComponent>(World, StackID))
	{
		// TODO: if scripted, try to find CanEquip function in the script
		// Return value will be true, false or nil. The latter is to proceed to the C++ logic.

		MaxStack = std::max(1u, pEquippable->MaxStack);

		const auto CheckBit = (1 << static_cast<int>(SlotType));
		if (pEquippable->IncludeBits & CheckBit) return MaxStack;
		if (pEquippable->ExcludeBits & CheckBit) return 0;
	}

	switch (SlotType)
	{
		case EEquipmentSlotType::HandItem:
		{
			// TODO: allow only Weapon or Shield, don't allow to equip to the slot blocked by a two-handed item
			return MaxStack;
		}
	}

	return 0;
}
//---------------------------------------------------------------------

}
