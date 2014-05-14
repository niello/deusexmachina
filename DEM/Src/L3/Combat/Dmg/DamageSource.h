#pragma once
#ifndef __IPG_DMG_SOURCE_H__
#define __IPG_DMG_SOURCE_H__

#include <StdDEM.h> 
//#include <Core/Object.h>

// Damage source produces DamageEffects and applies them at the right time to the right targets.
// Triggered by source-owning entity.

namespace Prop
{
	class CPropDestructible;
}

namespace Dmg
{

using namespace Prop;

//!!!not here!
enum ETargetType
{
	TGT_SELF,		// Applies effect to itself
	TGT_TARGET,		// To the specified target
	TGT_N_TARGETS,	// To the first N effect acceptors in the effect area
	TGT_AREA		// To the all effect acceptors in the effect area //!!!now only round area, need others!
}

//!!!define effect types - immediate, projectile! projectile params: isHoming, radius etc.

class CDamageSource //: public Core::CObject //???effect source?
{
public:

	//!!!getters-setters are needed at least for some members!
	CDamageEffect	Effect; //???ptr? array of effects?
	ETargetType		TargetType;
	float			MinDistance,	// Distance from source to acceptor
					MaxDistance;
	float			InnerRadius,	// Inner & outer radii of the effect area
					OuterRadius;
	int				Counter;		// -1 - infinite-looped source, 1 - once, 2 etc - N-times source
	float			ROF;			// Rate Of Fire, milliseconds between two effect productions
	//???smth like recharge? think about wpn implementation!

	CDamageSource();
};

}

#endif