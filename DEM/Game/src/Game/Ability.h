#pragma once
#ifndef __DEM_L2_GAME_ABILITY_H__
#define __DEM_L2_GAME_ABILITY_H__

#include <Data/Flags.h>
#include <Data/Params.h>

// Ability resembles a way to interact with a game object. User must select
// an ability and apply it to a target, so concrete action is determined.
// For example, "Repair" ability can be applied to Entity targets which
// resemble broken mechanical objects. Each of these broken objects reacts
// on "Repair" request by returning Action that contains game logic of
// repairing this actual object.

//!!!must have a way to know if ability is targeted on self or any other predetermined
//target, so that target can be automatically selected! ("self" scenario)

namespace Game
{
class IAction;

class CAbility
{
protected:

	CStrID			ID;
	IAction*		pAction;		// May be nullptr, in this case smart object must provide an action
	Data::CFlags	TargetFlags;	//???or RTTI list or CStrID list? use dynamic enum?
	Data::PParams	Params;

public:

	CAbility(): pAction(nullptr) {}

	bool		AcceptsTargetType(UPTR TargetTypeFlag) const { return TargetFlags.Is(TargetTypeFlag); }
	IAction*	GetAction() { return pAction; }
};

}

#endif
