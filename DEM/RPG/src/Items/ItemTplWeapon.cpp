#include "ItemTplWeapon.h"

#include "Item.h"
#include <Data/Params.h>
#include <Core/Factory.h>

namespace Items
{
__ImplementClass(Items::CItemTplWeapon, 'ITWP', Items::CItemTpl);

void CItemTplWeapon::Init(CStrID SID, const Data::CParams& Params)
{
	WpnClass = CStrID(Params.Get<CString>(CStrID("WpnClass"), CString::Empty).CStr());
	AmmoItemID = CStrID(Params.Get<CString>(CStrID("AmmoItemID"), CString::Empty).CStr());
	RangeMin = Params.Get<float>(CStrID("RangeMin"), 0.f);
	RangeMax = Params.Get<float>(CStrID("RangeMax"), 0.f);
	ROF = Params.Get<float>(CStrID("ROF"), 0.f);
	Ranged = Params.Get<bool>(CStrID("Ranged"), false);
	TwoHanded = Params.Get<bool>(CStrID("TwoHanded"), false);

	//???where to store defaults and does need to store them?
	const Data::CParams& Dmg = *Params.Get<Data::PParams>(CStrID("Dmg"));
	DmgType = (Dmg::EDmgType)Dmg.Get<int>(CStrID("Type"), 0);
	if (!Dmg.Get<int>(x, CStrID("x"))) x = Dmg.Get<int>(CStrID("X"), 1);
	if (!Dmg.Get<int>(y, CStrID("y"))) y = Dmg.Get<int>(CStrID("Y"), 6);
	if (!Dmg.Get<int>(z, CStrID("z"))) z = Dmg.Get<int>(CStrID("Z"), 0);

	// Can create CItemWeapon here if needed and init its fields from Params

	CItemTpl::Init(SID, Params);
}
//---------------------------------------------------------------------

};
