#pragma once
#include <Core/Object.h>
#include <Data/Metadata.h>
#include <Data/StringID.h>
#include <map>

// Stores an equipment of a character, including quick slots

namespace DEM::RPG
{

class CEquipmentScheme : public ::Core::CObject
{
	RTTI_CLASS_DECL(CEquipmentScheme, ::Core::CObject);

public:

	std::map<CStrID, size_t> Slots;
	std::map<CStrID, CStrID> SlotToType;

	void OnPostLoad()
	{
		for (const auto [Type, Count] : Slots)
		{
			if (!Count) continue;

			if (Count == 1)
				SlotToType.emplace(Type, Type);
			else
				for (size_t i = 1; i <= Count; ++i)
					SlotToType.emplace(CStrID((Type.ToString() + std::to_string(i)).c_str()), Type);
		}
	}
};

using PEquipmentScheme = Ptr<CEquipmentScheme>;

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::RPG::CEquipmentScheme>() { return "DEM::RPG::CEquipmentScheme"; }
template<> inline constexpr auto RegisterMembers<DEM::RPG::CEquipmentScheme>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CEquipmentScheme, 1, Slots)
	);
}

}
