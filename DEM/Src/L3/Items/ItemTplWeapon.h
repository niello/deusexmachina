#pragma once
#ifndef __IPG_ITEM_TPL_WEAPON_H__
#define __IPG_ITEM_TPL_WEAPON_H__

#include "ItemTpl.h" 
#include <Combat/Dmg/Damage.h>

// Template for weapon item. Contains weapon parameters that does not change per item instance.
// Weapon item can be equipped and used to inflict damage.

namespace Items
{

using namespace Dmg;

class CItemTplWeapon: public CItemTpl
{
	__DeclareClass(CItemTplWeapon);

protected:

public:

	//???what of this can change per instance? (Sharpness or smth)
	CStrID		WpnClass;	//???or store class ptr if WpnClass is structure? now it isn't, need only for role system
	CStrID		AmmoItemID;	// If empty, need not ammo
	float		RangeMin,
				RangeMax;
	float		ROF;		// Rate Of Fire, (milli?)seconds between two strikes. May depend on skill!
	bool		Ranged;		// Does emit projectiles? //???is wpnclass prop?
	bool		TwoHanded;	//???is wpnclass prop? per-item is more flexible
	
	//!!!???dmg to structure? is it dmgeffect or other struct?
	//!!!Dmg will be much more flexible & rule-based. now temporary solution:
	EDmgType	DmgType;
	int			x, y, z;	// Damage value, calculated as xdy+z
	
	virtual void Init(CStrID SID, const CParams& Params);
};

__RegisterClassInFactory(CItemTplWeapon);

}

#endif