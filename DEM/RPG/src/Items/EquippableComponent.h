#pragma once
#include <Data/Metadata.h>
#include <StdDEM.h>

// Generic equippable settings. They override implicit equipment rules like 'all weapons can be equipped to hands'.

namespace DEM::RPG
{

struct CEquippableComponent
{
	// One bit per EEquipmentSlotType
	U32 IncludeBits = 0;
	U32 ExcludeBits = 0;

	//???bool TryScript?
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::RPG::CEquippableComponent>() { return "DEM::RPG::CEquippableComponent"; }
template<> inline constexpr auto RegisterMembers<DEM::RPG::CEquippableComponent>()
{
	return std::make_tuple
	(
	);
}

}

/*
namespace DEM::Serialization
{

template<>
struct ParamsFormat<DEM::RPG::CEquippableComponent>
{
	static inline void Serialize(Data::CData& Output, DEM::RPG::CEquippableComponent Value)
	{
		//Output = ...;
	}

	static inline void Deserialize(const Data::CData& Input, DEM::RPG::CEquippableComponent& Value)
	{
		DeserializeDiff(Input, Value);
	}

	static inline bool SerializeDiff(Data::CData& Output, const DEM::RPG::CEquippableComponent& Value, const DEM::RPG::CEquippableComponent& BaseValue)
	{
		if (Value != BaseValue)
		{
			// Output = ...;
			return true;
		}

		return false;
	}

	static inline void DeserializeDiff(const Data::CData& Input, DEM::RPG::CEquippableComponent& Value)
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

		for (U32 i = 0; i < RPG::EEquipmentSlot::COUNT; ++i)
		{
			if (auto pParam = Desc->Find(CStrID(RPG::EEquipmentSlot_Name[i])))
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
*/
