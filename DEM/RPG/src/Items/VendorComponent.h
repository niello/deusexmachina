#pragma once
#include <Resources/Resource.h>
#include <Game/ECS/Entity.h>
#include <Data/Metadata.h>

// Allows an item to serve as a lockpick

namespace DEM::RPG
{

struct CGeneratedComponent {}; // TODO: can move to a separate header, "generated" flag is pretty universal, can be reused e.g. with enemies

struct CVendorComponent
{
	Game::HEntity        ContainerID;
	Resources::PResource ItemGeneratorAsset; // CItemList
	Resources::PResource CurrencyListAsset; // CItemList
	CStrID               ScriptAssetID;
	U32                  RegenerationPeriod = 0;
	U32                  LastGenerationTimestamp = 0;
	U32                  Money = 0;
	U32                  MinGeneratedMoney = 0;
	U32                  MaxGeneratedMoney = 0;
	std::optional<float> BuyFromVendorCoeff;
	std::optional<float> SellToVendorCoeff;
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<RPG::CVendorComponent>() { return "DEM::RPG::CVendorComponent"; }
template<> constexpr auto RegisterMembers<RPG::CVendorComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CVendorComponent, ContainerID),
		DEM_META_MEMBER_FIELD(RPG::CVendorComponent, ItemGeneratorAsset),
		DEM_META_MEMBER_FIELD(RPG::CVendorComponent, CurrencyListAsset),
		DEM_META_MEMBER_FIELD(RPG::CVendorComponent, ScriptAssetID),
		DEM_META_MEMBER_FIELD(RPG::CVendorComponent, RegenerationPeriod),
		DEM_META_MEMBER_FIELD(RPG::CVendorComponent, LastGenerationTimestamp),
		DEM_META_MEMBER_FIELD(RPG::CVendorComponent, Money),
		DEM_META_MEMBER_FIELD(RPG::CVendorComponent, MinGeneratedMoney),
		DEM_META_MEMBER_FIELD(RPG::CVendorComponent, MaxGeneratedMoney),
		DEM_META_MEMBER_FIELD(RPG::CVendorComponent, BuyFromVendorCoeff),
		DEM_META_MEMBER_FIELD(RPG::CVendorComponent, SellToVendorCoeff)
	);
}
static_assert(CMetadata<RPG::CVendorComponent>::ValidateMembers()); // FIXME: how to trigger in RegisterMembers?

}
