#include "PropWeapon.h"

#include <AI/Prop/PropActorBrain.h>
#include <Combat/Event/ObjDamageDone.h>
#include <AI/AIServer.h>
#include <Game/Mgr/EntityManager.h>
#include <Game/GameServer.h>
#include <Loading/EntityFactory.h>
#include <DB/DBServer.h>

namespace Attr
{
    DefineFloat(WpnROF);
    DefineFloat(WpnRangeMin);
    DefineFloat(WpnRangeMax);
    DefineFloat(WpnLastStrikeTime);
};

BEGIN_ATTRS_REGISTRATION(PropWeapon)
    RegisterFloatWithDefault(WpnROF, ReadOnly, 2.0f);
    RegisterFloatWithDefault(WpnRangeMin, ReadOnly, 0.1f);
    RegisterFloatWithDefault(WpnRangeMax, ReadOnly, 0.8f);
    RegisterFloatWithDefault(WpnLastStrikeTime, ReadOnly, -2.1f); //???!!! check bhv
END_ATTRS_REGISTRATION

namespace Properties
{
ImplementRTTI(Properties::CPropWeapon, Game::CProperty);
ImplementFactory(Properties::CPropWeapon);
ImplementPropertyStorage(CPropWeapon, 64);
RegisterProperty(CPropWeapon);

using namespace Event;

//!!!not here, in strid (static CStrID::Attack)!
static CStrID SidAttack;

CPropWeapon::CPropWeapon():
	x(1), y(6), z(0),
	DmgType(DT_WPN_THRUST)
{
	//!!!not here, in strid (static CStrID::Attack)!
	SidAttack = CStrID("Attack");
}
//---------------------------------------------------------------------

void CPropWeapon::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
    CProperty::GetAttributes(Attrs);
	Attrs.Append(Attr::WpnROF);
	Attrs.Append(Attr::WpnRangeMin);
	Attrs.Append(Attr::WpnRangeMax);
	Attrs.Append(Attr::WpnLastStrikeTime);
}
//---------------------------------------------------------------------

void CPropWeapon::Activate()
{
	Game::CProperty::Activate();

	//if (GetEntity()->IsActive()) SetupBrain(true);

	PROP_SUBSCRIBE_PEVENT(OnPropsActivated, CPropWeapon, OnPropsActivated);
	PROP_SUBSCRIBE_PEVENT(ChrStrike, CPropWeapon, OnChrStrike);
}
//---------------------------------------------------------------------

void CPropWeapon::Deactivate()
{
	UNSUBSCRIBE_EVENT(OnPropsActivated);
	UNSUBSCRIBE_EVENT(ChrStrike);

	//if (!GetEntity()->IsDeactivating()) SetupBrain(false);

	Game::CProperty::Deactivate();
}
//---------------------------------------------------------------------

bool CPropWeapon::OnPropsActivated(const CEventBase& Event)
{
	//SetupBrain(true);
	OK;
}
//---------------------------------------------------------------------

bool CPropWeapon::OnChrStrike(const CEventBase& Event)
{
	//???or get from attr?
	//CStrID TargetEntityID = (*((CEvent&)Event).Params).Get<CStrID>(CStrID("TargetEntityID"));
	//
	//if (EntityMgr->ExistsEntityByID(TargetEntityID))
	//	Strike(*EntityMgr->GetEntityByID(TargetEntityID));

	OK;
}
//---------------------------------------------------------------------

//???destructible prop as arg?
void CPropWeapon::Strike(Game::CEntity& Target)
{
    GetEntity()->Set<float>(Attr::WpnLastStrikeTime, (float)GameSrv->GetTime());

	//!!!check hit (accurecy etc)!

	Ptr<ObjDamageDone> Event = ObjDamageDone::Create();
	Event->EntDamager = GetEntity()->GetUniqueID();
	Event->Type = DmgType;

	//!!!calc damage with all modifiers etc based on calculation rule!
	//calc rule may involve target params, like "relative" that damages in % of target's health

	Event->Amount = z;
	for (int i = 0; i < x; i++)
		Event->Amount += n_rand_int(1, y);

	//!!!set flags like SuppresResistance etc!

#ifdef _DEBUG
	n_printf("CEntity \"%s\" : Hits entity \"%s\"; Damage = %d\n",
		GetEntity()->GetUniqueID(),
		Target.GetUniqueID(),
		Event->Amount);
#endif

	Target.FireEvent(*Event);
}
//---------------------------------------------------------------------

} // namespace Properties