#pragma once
#ifndef __IPG_DAMAGE_H__
#define __IPG_DAMAGE_H__

#include <StdDEM.h> 

// Damage system base enums etc.

namespace Dmg
{

enum EDmgType //???dynamic enumeration of StrIDs?
{
	DT_WPN_THRUST,	// Thrust damage from basic weapon (steel/iron/wood etc) //???special dmg for bullets?
	DT_WPN_CHOP,	// Chop =//=
	DT_WPN_CRUSH,	// Crush =//=
	DT_FIRE,
	DT_MAGIC,		// Raw magical energy
	DT_POISON
};

}

#endif