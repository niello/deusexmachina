#pragma once
#ifndef __DEM_L2_GAME_ABILITY_H__
#define __DEM_L2_GAME_ABILITY_H__

#include <Data/Flags.h>
#include <Data/Params.h>

// Ability determines an interaction context. Actor may have many abilities,
// each can be applied to targets to launch execution of an action.
// For example, "Repair" is an ability that can be applied to some broken objects
// to execute actions that will effect in repairing of that objects.

namespace Game
{
class CGameAction; //???rename to CAction later?

class CAbility
{
protected:

	CStrID			ID;
	CGameAction*	pAction;
	Data::CFlags	TargetFlags; //???or RTTI list or CStrID list? use dynamic enum?
	Data::PParams	Params;

public:

	CAbility(): pAction(NULL) {}

	bool	AcceptsTargetType(UPTR TargetTypeFlag) const { return TargetFlags.Is(TargetTypeFlag); }
};

}

#endif
