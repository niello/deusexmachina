#pragma once
#include <Data/Metadata.h>
#include <Character/ModifiableParameter.h> // FIXME: not character but stats?
#include <Combat/Damage.h>

// An entity with a destructible component will be logically destroyed (killed) when it runs out of HP

namespace DEM::RPG
{

struct CDestructibleComponent
{
	CModifiableParameter<int>               HP = 0;
	CModifiableParameter<CDamageAbsorption> DamageAbsorption;

	Events::CSignal<void(int)> OnHit;
	Events::CSignal<void()>    OnDestroyed;

	// TODO: resistances and immunities. here or in a separate component with hit zones?

	//!!!FIXME: need something like SerializeAs<CDamageAbsorption>! Transparent conversion from and to the serializable type. Or provide explicit methods for get and set.
	// Could use it instead of RegisterMembers, i.e. like it has only one member - itself. Metadata::Copy will work correctly then.
	// Or maybe that must be per field, not per type?
	//!!!FIXME: serialization fails to compile if move setter is used, although it should benefit from it!
	//void SetDamageAbsorption(CDamageAbsorption&& Value) { DamageAbsorption = std::move(Value); }
	void SetDamageAbsorption(const CDamageAbsorption& Value) { DamageAbsorption = Value; }
	const CDamageAbsorption& GetDamageAbsorption() const { return DamageAbsorption.GetBaseValue(); }
	void SetHP(int Value) { HP = Value; }
	int GetHP() const { return HP.GetBaseValue(); }
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::RPG::CDestructibleComponent>() { return "DEM::RPG::CDestructibleComponent"; }
template<> inline constexpr auto RegisterMembers<DEM::RPG::CDestructibleComponent>()
{
	return std::make_tuple
	(
		//DEM_META_MEMBER_FIELD(RPG::CDestructibleComponent, 1, HP),
		Member<RPG::CDestructibleComponent, int>(1, "HP", &RPG::CDestructibleComponent::GetHP, &RPG::CDestructibleComponent::SetHP),
		Member<RPG::CDestructibleComponent, RPG::CDamageAbsorption>(2, "DamageAbsorption", &RPG::CDestructibleComponent::GetDamageAbsorption, &RPG::CDestructibleComponent::SetDamageAbsorption)
	);
}

}
