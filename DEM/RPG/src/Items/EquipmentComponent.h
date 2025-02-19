#pragma once
#include <Items/EquipmentScheme.h>
#include <Game/ECS/Entity.h>

// Stores an equipment of a character

namespace DEM::RPG
{

struct CHandSlot
{
	Game::HEntity ItemStackID;
	bool          Unsheathed = false;
};

struct CEquipmentComponent
{
	std::map<CStrID, Game::HEntity> Equipment;
	std::vector<Game::HEntity>      QuickSlots;
	CStrID                          SchemeID; // FIXME: ID to resource directly!
	PEquipmentScheme                Scheme;
	std::unique_ptr<CHandSlot[]>    Hands;
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<DEM::RPG::CEquipmentComponent>() { return "DEM::RPG::CEquipmentComponent"; }
template<> constexpr auto RegisterMembers<DEM::RPG::CEquipmentComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CEquipmentComponent, Equipment),
		DEM_META_MEMBER_FIELD(RPG::CEquipmentComponent, QuickSlots),
		DEM_META_MEMBER_FIELD(RPG::CEquipmentComponent, SchemeID)
	);
}

}

namespace DEM::Serialization
{

template<>
struct ParamsFormat<DEM::RPG::CEquipmentComponent>
{
	static inline void Serialize(Data::CData& Output, const DEM::RPG::CEquipmentComponent& Value)
	{
		//Output = ...;
	}

	static inline void Deserialize(const Data::CData& Input, DEM::RPG::CEquipmentComponent& Value)
	{
		DeserializeDiff(Input, Value);
	}

	static inline bool SerializeDiff(Data::CData& Output, const DEM::RPG::CEquipmentComponent& Value, const DEM::RPG::CEquipmentComponent& BaseValue)
	{
		if (Value != BaseValue)
		{
			// Output = ...;
			return true;
		}

		return false;
	}

	static inline void DeserializeDiff(const Data::CData& Input, DEM::RPG::CEquipmentComponent& Value)
	{
		Data::PParams Desc = Input.GetValue<Data::PParams>();
		if (!Desc) return;

		Value.QuickSlots.resize(Desc->Get<int>(CStrID("QuickSlotCount"), 0));

		// FIXME: fall back to default code
		Value.SchemeID = Desc->Get<CStrID>(CStrID("SchemeID"), {});

		// FIXME: fall back to default code
		if (auto QuickSlotsDesc = Desc->Find(CStrID("QuickSlots")))
			DEM::ParamsFormat::Deserialize(QuickSlotsDesc->GetRawValue(), Value.QuickSlots);

		// FIXME: fall back to default code
		if (auto EquipmentDesc = Desc->Find(CStrID("Equipment")))
			DEM::ParamsFormat::Deserialize(EquipmentDesc->GetRawValue(), Value.Equipment);
	}
};

}
