#pragma once
#ifndef __IPG_DMG_EFFECT_H__
#define __IPG_DMG_EFFECT_H__

#include "Damage.h"
//#include <Core/Object.h>

// Damage effect causes damage on Destructible object according to some rules incorporated into this effect.
// Sends or feeds with data ObjDamageDone message.
// Triggered by acceptor.

namespace Prop
{
	class CPropDestructible;
}

namespace Dmg
{

using namespace Prop;

//???struct?
class CDamageEffect //???public CEffect? //: public Core::CObject
{
public:

	// Damage-specific
	//!!!different laws may be needed! like lerp, constant, relative to health etc, not only xdy+z
	int			x, y, z;	// Damage value, calculated as xdy+z
	EDmgType	Type;
	
	// All effects
	float		ROF;		// Rate Of Fire, milliseconds between two effect applications
	int			Counter;	// -1 - infinite-looped effect, 1 - once, 2 etc - N-times effect

	CDamageEffect();

	//???or get/generate, not apply?
	void ApplyRandomDamage(CPropDestructible* pAcceptor);
	void ApplyMinimalDamage(CPropDestructible* pAcceptor);
	void ApplyMaximalDamage(CPropDestructible* pAcceptor);
};

}

#endif