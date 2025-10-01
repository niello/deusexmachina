#pragma once
#include <Data/Metadata.h>
#include <Combat/Damage.h>

// An entity with a destructible component will be logically destroyed (killed) when it runs out of HP

namespace DEM::RPG
{

struct CDestructibleComponent
{
	int                                         HP = 0;
	std::map<CStrID, CZoneDamageAbsorptionStat> DamageAbsorption;

	Events::CSignal<void(int)>         OnHit;
	Events::CSignal<void(EDamageType)> OnDestroyed;

	// TODO: resistances and immunities. here or in a separate component with hit zones?
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<DEM::RPG::CDestructibleComponent>() { return "DEM::RPG::CDestructibleComponent"; }
template<> constexpr auto RegisterMembers<DEM::RPG::CDestructibleComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CDestructibleComponent, HP),
		DEM_META_MEMBER_FIELD(RPG::CDestructibleComponent, DamageAbsorption)
	);
}

}
