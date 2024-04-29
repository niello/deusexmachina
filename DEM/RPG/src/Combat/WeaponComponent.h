#pragma once
#include <Data/Metadata.h>
#include <Character/ModifiableParameter.h> // FIXME: not character but stats?
#include <Combat/Damage.h>

// Weapon is typically an item component which makes it capable of attacking targets and inflicting damage

namespace DEM::RPG
{

struct CDamageData
{
	EDamageType Type = EDamageType::Bludgeoning;
	uint8_t     x = 1;
	uint8_t     y = 1;
	int8_t      z = 0;
};

struct CWeaponComponent
{
	//!!!DBG TMP!
	CDamageData Damage;

	bool Big = false;
};

}

namespace DEM::Serialization
{

//!!!FIXME: need generic serialization for all enums!
template<>
struct ParamsFormat<DEM::RPG::EDamageType>
{
	static inline void Serialize(Data::CData& Output, DEM::RPG::EDamageType Value)
	{
		switch (Value)
		{
			case DEM::RPG::EDamageType::Piercing: Output = CString("Piercing"); return;
			case DEM::RPG::EDamageType::Slashing: Output = CString("Slashing"); return;
			case DEM::RPG::EDamageType::Bludgeoning: Output = CString("Bludgeoning"); return;
			case DEM::RPG::EDamageType::Energetic: Output = CString("Energetic"); return;
			case DEM::RPG::EDamageType::Chemical: Output = CString("Chemical"); return;
			default: Output = {}; return;
		}
	}

	static inline void Deserialize(const Data::CData& Input, DEM::RPG::EDamageType& Value)
	{
		if (!Input.IsA<CString>()) return;

		const std::string_view TypeStr = Input.GetValue<CString>().CStr();
		if (TypeStr == "Piercing") Value = DEM::RPG::EDamageType::Piercing;
		else if (TypeStr == "Slashing") Value = DEM::RPG::EDamageType::Slashing;
		else if (TypeStr == "Bludgeoning") Value = DEM::RPG::EDamageType::Bludgeoning;
		else if (TypeStr == "Energetic") Value = DEM::RPG::EDamageType::Energetic;
		else if (TypeStr == "Chemical") Value = DEM::RPG::EDamageType::Chemical;
	}
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::RPG::CDamageData>() { return "quaternion"; }
template<> inline constexpr auto RegisterMembers<DEM::RPG::CDamageData>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CDamageData, 1, Type),
		DEM_META_MEMBER_FIELD(RPG::CDamageData, 2, x),
		DEM_META_MEMBER_FIELD(RPG::CDamageData, 3, y),
		DEM_META_MEMBER_FIELD(RPG::CDamageData, 4, z)
	);
}

template<> inline constexpr auto RegisterClassName<DEM::RPG::CWeaponComponent>() { return "DEM::RPG::CWeaponComponent"; }
template<> inline constexpr auto RegisterMembers<DEM::RPG::CWeaponComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CWeaponComponent, 1, Damage),
		DEM_META_MEMBER_FIELD(RPG::CWeaponComponent, 2, Big)
	);
}

}
