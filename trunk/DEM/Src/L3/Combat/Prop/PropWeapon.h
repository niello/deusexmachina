#pragma once
#ifndef __IPG_PROP_WEAPON_H__
#define __IPG_PROP_WEAPON_H__

#include <game/property.h>
#include <db/AttrID.h>

#include <Combat/Dmg/Damage.h>

// Adds ability to attack destructible objects. Also stores parameters of currently equipped weapon,
// either item-based or native for the creature represented by entity.
// This property extends and tunes ActorBrain so it become knowing algorithm for Attack.

// Adds Actor actions available:
// - Attack

namespace Attr
{
	DeclareFloat(WpnROF);				// Current actual Rate Of Fire (with all modifiers applied)
	DeclareFloat(WpnRangeMin);			// Current actual weapon range minimum (with all modifiers applied)
	DeclareFloat(WpnRangeMax);			// Current actual weapon range maximum (with all modifiers applied)
	DeclareFloat(WpnLastStrikeTime);	// Game time of the last strike
};

namespace Properties
{

using namespace Dmg;
using namespace Events;

class CPropWeapon: public Game::CProperty
{
	DeclareRTTI;
	DeclareFactory(CPropWeapon);
	DeclarePropertyStorage;

protected:

	//void SetupBrain(bool Active);

	DECLARE_EVENT_HANDLER(OnPropsActivated, OnPropsActivated);
	DECLARE_EVENT_HANDLER(ChrStrike, OnChrStrike);

public:

	// Damage params
	//!!!on set Y change die sides!
	//???move from here?
	int			x, y, z;		// Damage value, calculated as xdy+z
	EDmgType	DmgType;

	// Weapon params (this + Damage params will be moved to Dmg or Items as wpnparams struct)
	// !!!don't store here! here attributes store this data
	//float		MinDistance,	// Distance from source to acceptor
	//			MaxDistance;
	//float		ROF;			// Rate Of Fire, (milli?)seconds between two strikes. May depend on skill!
	//weapon's default ROF & final ROF can be different!
	//effects applied on hit
	bool		NeedAmmo; //???item param? or there can be creature-native ammo-requiring weapon?

	// Item params
	// item ref
	// ammo ref

	// Role system params
	// attack, damage etc role system params (chance to hit modifier, strength bonus etc)

	CPropWeapon();
	//virtual ~CPropWeapon();

	virtual void	GetAttributes(nArray<DB::CAttrID>& Attrs);
	virtual void	Activate();
	virtual void	Deactivate();

	//SetWeapon(Item)
	//SetWeapon(WpnDesc)

	//!!!ranged weapons should behave differently, using Projectile (universal for ranged, magic etc)!
	//EmitProjectile(Target) or bool flag-based inside Strike?
	void			Strike(Game::CEntity& Target); //???destructible prop as arg?
};

RegisterFactory(CPropWeapon);

}

#endif