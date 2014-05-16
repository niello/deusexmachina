#include "PropWeapon.h"

#include <AI/PropActorBrain.h>
#include <Combat/Event/ObjDamageDone.h>
#include <AI/AIServer.h>
#include <Game/EntityManager.h>
#include <Game/GameServer.h>

//BEGIN_ATTRS_REGISTRATION(PropWeapon)
//    RegisterFloatWithDefault(WpnROF, ReadOnly, 2.0f);
//    RegisterFloatWithDefault(WpnRangeMin, ReadOnly, 0.1f);
//    RegisterFloatWithDefault(WpnRangeMax, ReadOnly, 0.8f);
//    RegisterFloatWithDefault(WpnLastStrikeTime, ReadOnly, -2.1f); //???!!! check bhv
//END_ATTRS_REGISTRATION

namespace Prop
{
__ImplementClass(Prop::CPropWeapon, 'PWPN', Game::CProperty);
__ImplementPropertyStorage(CPropWeapon);

using namespace Event;

//!!!not here, in strid (static CStrID::Attack)!
static CStrID SidAttack;

CPropWeapon::CPropWeapon():
	x(1), y(6), z(0),
	DmgType(Dmg::DT_WPN_THRUST)
{
	//!!!not here, in strid (static CStrID::Attack)!
	SidAttack = CStrID("Attack");
}
//---------------------------------------------------------------------

bool CPropWeapon::InternalActivate()
{
	//if (GetEntity()->IsActive()) SetupBrain(true);

	PROP_SUBSCRIBE_PEVENT(ChrStrike, CPropWeapon, OnChrStrike);
	OK;
}
//---------------------------------------------------------------------

void CPropWeapon::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(ChrStrike);

	//if (!GetEntity()->IsDeactivating()) SetupBrain(false);

}
//---------------------------------------------------------------------

bool CPropWeapon::OnChrStrike(const Events::CEventBase& Event)
{
	//???or get from attr?
	//CStrID TargetEntityID = (*((CEvent&)Event).Params).Get<CStrID>(CStrID("TargetEntityID"));
	//
	//if (EntityMgr->EntityExists(TargetEntityID))
	//	Strike(*EntityMgr->GetEntity(TargetEntityID));

	OK;
}
//---------------------------------------------------------------------

//???destructible prop as arg?
void CPropWeapon::Strike(Game::CEntity& Target)
{
    GetEntity()->SetAttr<float>(CStrID("WpnLastStrikeTime"), (float)GameSrv->GetTime());

	//!!!check hit (accurecy etc)!

	Ptr<ObjDamageDone> Event = ObjDamageDone::CreateInstance();
	Event->EntDamager = GetEntity()->GetUID();
	Event->Type = DmgType;

	//!!!calc damage with all modifiers etc based on calculation rule!
	//calc rule may involve target params, like "relative" that damages in % of target's health

	Event->Amount = z;
	for (int i = 0; i < x; i++)
		Event->Amount += n_rand_int(1, y);

	//!!!set flags like SuppresResistance etc!

#ifdef _DEBUG
	Sys::Log("CEntity \"%s\" : Hits entity \"%s\"; Damage = %d\n",
		GetEntity()->GetUID(),
		Target.GetUID(),
		Event->Amount);
#endif

	Target.FireEvent(*Event);
}
//---------------------------------------------------------------------

} // namespace Prop