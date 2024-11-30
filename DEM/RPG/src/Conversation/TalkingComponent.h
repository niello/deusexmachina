#pragma once
#include <Resources/Resource.h>
#include <Data/Metadata.h>

// Enables talking with this entity

namespace DEM::RPG
{

struct CTalkingComponent
{
	Resources::PResource Asset; // CFlowAsset
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<DEM::RPG::CTalkingComponent>() { return "DEM::RPG::CTalkingComponent"; }
template<> constexpr auto RegisterMembers<DEM::RPG::CTalkingComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CTalkingComponent, Asset)
	);
}

}
