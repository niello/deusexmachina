#pragma once
#include <Data/Metadata.h>
#include <StdDEM.h>

// Stores an equipment of a character, including quick slots, for a Shantara2 role system

namespace DEM::Sh2
{

enum EEquipmentSlot
{
	Body = 0,
	Shoulders,
	Head,
	Arms,
	Hands,
	Legs,
	Feet,
	Belt,
	Backpack,
	Neck,
	BraceletLeft,
	BraceletRight,
	Ring1Left,
	Ring1Right,
	Ring2Left,
	Ring2Right,

	COUNT
};

struct CEquipmentComponent
{
	Game::HEntity              Equipment[EEquipmentSlot::COUNT];
	std::vector<Game::HEntity> QuickSlots;
	U32                        SlotEnabledBits = 0;
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::Sh2::CEquipmentComponent>() { return "DEM::Sh2::CEquipmentComponent"; }
template<> inline constexpr auto RegisterMembers<DEM::Sh2::CEquipmentComponent>()
{
	//!!!FIXME: check what metadata is needed here! Maybe need specific getters and setters instead of raw fields.
	return std::make_tuple
	(
		//DEM_META_MEMBER_FIELD(Sh2::CEquipmentComponent, 1, Equipment), // TODO: support C arrays
		DEM_META_MEMBER_FIELD(Sh2::CEquipmentComponent, 2, QuickSlots),
		DEM_META_MEMBER_FIELD(Sh2::CEquipmentComponent, 3, SlotEnabledBits)
	);
}

}

namespace DEM::Serialization
{

template<>
struct ParamsFormat<DEM::Sh2::CEquipmentComponent>
{
	static inline void Serialize(Data::CData& Output, DEM::Sh2::CEquipmentComponent Value)
	{
		//Output = static_cast<int>(Value.Equipment[0]);
	}

	static inline void Deserialize(const Data::CData& Input, DEM::Sh2::CEquipmentComponent& Value)
	{
		//Value.Equipment[0] = DEM::Game::HEntity{ static_cast<DEM::Game::HEntity::TRawValue>(Input.GetValue<int>()) };
	}

	static inline bool SerializeDiff(Data::CData& Output, const DEM::Sh2::CEquipmentComponent& Value, const DEM::Sh2::CEquipmentComponent& BaseValue)
	{
		return false;
		//if (Value == BaseValue) return false;
		//Output = static_cast<int>(Value.Equipment[0]);
		//return true;
	}

	static inline void DeserializeDiff(const Data::CData& Input, DEM::Sh2::CEquipmentComponent& Value)
	{
		Data::PParams Desc = Input.GetValue<Data::PParams>();
		if (!Desc) return;

		Value.QuickSlots.resize(Desc->Get<int>(CStrID("QuickSlotCount")));

		if (Data::PDataArray QuickSlotsDesc = Desc->Get<Data::PDataArray>(CStrID("QuickSlots")))
		{
			const UPTR UsedSlotCount = std::min(Value.QuickSlots.size(), QuickSlotsDesc->GetCount());
			for (UPTR i = 0; i < UsedSlotCount; ++i)
			{
				const auto& SlotData = QuickSlotsDesc->Get(i);
				Value.QuickSlots[i] = SlotData.IsVoid() ?
					Game::HEntity{} :
					Game::HEntity{ static_cast<Game::HEntity::TRawValue>(SlotData.GetValue<int>()) };
			}
		}

		// TODO: read equipment
	}
};

}
