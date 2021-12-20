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
	BraceletMain,
	BraceletOff,
	Ring1Main,
	Ring1Off,
	Ring2Main,
	Ring2Off,
	MainHandA,
	OffHandA,
	MainHandB,
	OffHandB,

	COUNT
};
constexpr const char* EEquipmentSlot_Name[] =
{
	"Body",
	"Shoulders",
	"Head",
	"Arms",
	"Hands",
	"Legs",
	"Feet",
	"Belt",
	"Backpack",
	"Neck",
	"BraceletMain",
	"BraceletOff",
	"Ring1Main",
	"Ring1Off",
	"Ring2Main",
	"Ring2Off",
	"MainHandA",
	"OffHandA",
	"MainHandB",
	"OffHandB"
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
		//Output = ...;
	}

	static inline void Deserialize(const Data::CData& Input, DEM::Sh2::CEquipmentComponent& Value)
	{
		DeserializeDiff(Input, Value);
	}

	static inline bool SerializeDiff(Data::CData& Output, const DEM::Sh2::CEquipmentComponent& Value, const DEM::Sh2::CEquipmentComponent& BaseValue)
	{
		if (Value != BaseValue)
		{
			// Output = ...;
			return true;
		}

		return false;
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

		Value.SlotEnabledBits = 0;

		for (U32 i = 0; i < Sh2::EEquipmentSlot::COUNT; ++i)
		{
			if (auto pParam = Desc->Find(CStrID(Sh2::EEquipmentSlot_Name[i])))
			{
				const auto& SlotData = pParam->GetRawValue();
				Value.Equipment[i] = SlotData.IsVoid() ?
					Game::HEntity{} :
					Game::HEntity{ static_cast<Game::HEntity::TRawValue>(SlotData.GetValue<int>()) };
				Value.SlotEnabledBits |= (1 << i);
			}
		}
	}
};

}
