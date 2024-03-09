#pragma once
#include <Items/ItemStackComponent.h>
#include <Items/EquipmentComponent.h>
#include <Items/EquippedComponent.h>
#include <Character/ModifiableParameter.h>
#include <Game/ECS/GameWorld.h>

// Utilities and algorithms for item manipulation
// NB: storage manipulation functions maintain data integrity inside the single storage, but not across them.
// I.e. if the item is moved inside one container or equipment, it will never be lost or duplicated. If the item
// is moved from one container (or other storage) to another then the calling code is obliged to maintain integrity.

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

//???TODO: move to more appropriate place?
template<typename T>
class CIncreaseModifier : public CParameterModifier<T>
{
public:

	T   _Increase = {};
	I32 _Priority = 0;

	CIncreaseModifier(T Increase = {}, I32 Priority = 0) : _Increase(Increase), _Priority(Priority) {}

	virtual I32  GetPriority() const override { return _Priority; }
	virtual bool Apply(T& Value) override { Value += _Increase; return true; }
};

//???TODO: move to more appropriate place?
template<typename TModifier, typename... TArgs>
void AddEquipmentModifier(CEquippedComponent& Source, CModifiableParameter<typename TModifier::TParam>& Param, TArgs&&... Args)
{
	auto Mod = MakePtr<TModifier>(std::forward<TArgs>(Args)...);
	Source.Modifiers.push_back(Mod);
	Param.AddModifier(std::move(Mod));
	//Param.UpdateFinalValue();
}
//---------------------------------------------------------------------

inline Game::HEntity GetEquippedStack(const CEquipmentComponent& Component, CStrID SlotID)
{
	if (!SlotID) return {};

	auto It = Component.Equipment.find(SlotID);
	if (It != Component.Equipment.cend()) return It->second;

	if (SlotID.CStr()[0] == 'Q')
	{
		const size_t QIndex = static_cast<size_t>(std::atoi(SlotID.CStr() + 1) - 1);
		if (QIndex < Component.QuickSlots.size()) return Component.QuickSlots[QIndex];
	}

	return {};
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

CStrID GetQuickSlotID(size_t SlotIndex);
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
std::pair<CStrID, EItemStorage> FindMainOccupiedSlot(const Game::CGameWorld& World, const CEquipmentComponent& Equipment, Game::HEntity StackID, EItemStorage Storage = EItemStorage::None);
std::pair<CStrID, EItemStorage> FindMainOccupiedSlot(const Game::CGameWorld& World, Game::HEntity EntityID, Game::HEntity StackID, EItemStorage Storage = EItemStorage::None);
void ScheduleStackReequipment(Game::CGameWorld& World, Game::HEntity StackID, EItemStorage Storage = EItemStorage::None, CStrID SlotID = CStrID::Empty);
void ScheduleReequipment(Game::CGameWorld& World, Game::HEntity ItemID);
Game::HEntity GetEquippedStack(Game::CGameWorld& World, Game::HEntity EntityID, CStrID SlotID);
bool IsItemEquipped(Game::CGameWorld& World, Game::HEntity EntityID, Game::HEntity ItemProtoID);
bool IsItemStackEquipped(const CEquipmentComponent& Component, Game::HEntity StackID);
bool IsItemStackEquipped(Game::CGameWorld& World, Game::HEntity EntityID, Game::HEntity StackID);
void CalcEquipmentStats(Game::CGameWorld& World, Game::HEntity EntityID, CContainerStats& OutStats);

CStrID GetHandPseudoSlotID(size_t HandIndex);
bool EquipItemToHand(Game::CGameWorld& World, Game::HEntity EntityID, size_t HandIndex, CStrID SlotID);

U32 AddItemsToLocation(Game::CGameWorld& World, Game::HEntity ItemProtoID, U32 Count, CStrID LevelID, const rtm::qvvf& Tfm, float MergeRadius);
std::pair<Game::HEntity, U32> MoveItemsFromLocation(Game::CGameWorld& World, Game::HEntity StackID, U32 Count = std::numeric_limits<U32>().max());
std::pair<U32, bool> MoveItemsToLocationSlot(Game::CGameWorld& World, std::vector<Game::HEntity>& GroundItems, size_t SlotIndex, Game::HEntity StackID, U32 Count, CStrID LevelID, const rtm::qvvf& Tfm, bool Merge = true, Game::HEntity* pReplaced = nullptr);
std::pair<U32, bool> MoveItemsToLocation(Game::CGameWorld& World, Game::HEntity StackID, U32 Count, CStrID LevelID, const rtm::qvvf& Tfm, float MergeRadius = 0.f);
Game::HEntity MoveWholeStackToLocation(Game::CGameWorld& World, Game::HEntity StackID, CStrID LevelID, const rtm::qvvf& Tfm, float MergeRadius = 0.f);
void ClearLocationSlot(Game::CGameWorld& World, std::vector<Game::HEntity>& GroundItems, size_t SlotIndex);
U32 RemoveItemsFromLocation(Game::CGameWorld& World, Game::HEntity StackID, U32 Count);
void CleanupItemsFromLocation(Game::CGameWorld& World, Game::HEntity StackID);
bool AddItemVisualsToLocation(Game::CGameWorld& World, Game::HEntity StackID, const rtm::qvvf& Tfm);
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
