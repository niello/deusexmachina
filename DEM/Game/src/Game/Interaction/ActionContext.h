#pragma once
#ifndef __DEM_L2_GAME_ACTION_CONTEXT_H__
#define __DEM_L2_GAME_ACTION_CONTEXT_H__

#include <Data/Params.h>

// Action context describes who and how interacts with the target.
// Context supports arbitrary params that can be filled by the game.

namespace Game
{
class CAbility;
class ITarget;

struct CActionContext
{
	CAbility*		pAbility;
	ITarget*		pTarget;
	Data::PParams	Params;

	CActionContext(): pAbility(nullptr), pTarget(nullptr) {}
};

}

#endif
