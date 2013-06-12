#pragma once
#ifndef __IPG_PROP_WEAPON_H__
#define __IPG_PROP_WEAPON_H__

#include <Game/Property.h>
#include <Combat/Dmg/Damage.h>

// Adds ability to attack destructible objects. Also stores parameters of currently equipped weapon,
// either item-based or native for the creature represented by entity.
// This property extends and tunes ActorBrain so it become knowing algorithm for Attack.

// Adds Actor actions available:
// - Attack

namespace Prop
{

class CPropWeapon: public Game::CProperty
{
	__DeclareClass(CPropWeapon);
	__DeclarePropertyStorage;

protected:

	virtual bool InternalActivate();
	virtual void InternalDeactivate();
	//void SetupBrain(bool Active);

	DECLARE_EVENT_HANDLER(ChrStrike, OnChrStrike);

public:

	// Damage params
	//!!!on set Y change die sides!
	//???move from here?
	int				x, y, z;		// Damage value, calculated as xdy+z
	Dmg::EDmgType	DmgType;

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

	//SetWeapon(Item)
	//SetWeapon(WpnDesc)

	//!!!ranged weapons should behave differently, using Projectile (universal for ranged, magic etc)!
	//EmitProjectile(Target) or bool flag-based inside Strike?
	void			Strike(Game::CEntity& Target); //???destructible prop as arg?
};

}

#endif