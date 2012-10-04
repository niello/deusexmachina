#include "ItemTplWeapon.h"

#include "Item.h"
#include <Data/Params.h>

namespace Items
{
//ImplementRTTI(Items::CItemTplWeapon, Items::CItemTpl);
ImplementFactory(Items::CItemTplWeapon);

void CItemTplWeapon::Init(CStrID SID, const CParams& Params)
{
	WpnClass = CStrID(Params.Get<nString>(CStrID("WpnClass"), "").Get());
	AmmoItemID = CStrID(Params.Get<nString>(CStrID("AmmoItemID"), "").Get());
	RangeMin = Params.Get<float>(CStrID("RangeMin"), 0.f);
	RangeMax = Params.Get<float>(CStrID("RangeMax"), 0.f);
	ROF = Params.Get<float>(CStrID("ROF"), 0.f);
	Ranged = Params.Get<bool>(CStrID("Ranged"), false);
	TwoHanded = Params.Get<bool>(CStrID("TwoHanded"), false);

	//???where to store defaults and does need to store them?
	const CParams& Dmg = *Params.Get<PParams>(CStrID("Dmg"));
	DmgType = (EDmgType)Dmg.Get<int>(CStrID("Type"), 0);
	if (!Dmg.Get<int>(x, CStrID("x"))) x = Dmg.Get<int>(CStrID("X"), 1);
	if (!Dmg.Get<int>(y, CStrID("y"))) y = Dmg.Get<int>(CStrID("Y"), 6);
	if (!Dmg.Get<int>(z, CStrID("z"))) z = Dmg.Get<int>(CStrID("Z"), 0);

	// Can create CItemWeapon here if needed and init its fields from Params

	CItemTpl::Init(SID, Params);
}
//---------------------------------------------------------------------

};
