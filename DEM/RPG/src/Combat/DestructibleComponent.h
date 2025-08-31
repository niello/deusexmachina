#pragma once
#include <Data/Metadata.h>
#include <Character/ModifiableParameter.h> // FIXME: not character but stats?
#include <Combat/Damage.h>

// An entity with a destructible component will be logically destroyed (killed) when it runs out of HP

namespace DEM::RPG
{

struct CDestructibleComponent
{
	CModifiableParameter<int>               HP = 0; //!!!TODO: to plain int or float!
	std::map<CStrID, CZoneDamageAbsorption> DamageAbsorption;

	Events::CSignal<void(int)>         OnHit;
	Events::CSignal<void(EDamageType)> OnDestroyed;

	// TODO: resistances and immunities. here or in a separate component with hit zones?

	void SetHP(int Value) { HP = Value; }
	int GetHP() const { return HP.GetBaseValue(); }
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<DEM::RPG::CDestructibleComponent>() { return "DEM::RPG::CDestructibleComponent"; }
template<> constexpr auto RegisterMembers<DEM::RPG::CDestructibleComponent>()
{
	return std::make_tuple
	(
		Member<RPG::CDestructibleComponent, int>(1, "HP", &RPG::CDestructibleComponent::GetHP, &RPG::CDestructibleComponent::SetHP),
		//DEM_META_MEMBER_FIELD(RPG::CDestructibleComponent, HP),
		DEM_META_MEMBER_FIELD(RPG::CDestructibleComponent, DamageAbsorption)
	);
}

}
