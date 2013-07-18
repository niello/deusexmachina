#pragma once
#ifndef __DEM_L3_PROP_TRANSITION_ZONE_H__
#define __DEM_L3_PROP_TRANSITION_ZONE_H__

#include <Game/Property.h>

// Transition zone allows characters (both player-controlled & NPC) to travel between locations.
// Provides data for IAO Travel action.

//???need property or can provide only attributes?
//Prop can add IAO name like "Travel to <Level Name>"

namespace Prop
{

class CPropTransitionZone: public Game::CProperty
{
	__DeclareClass(CPropTransitionZone);
	__DeclarePropertyStorage;

protected:

	virtual bool InternalActivate();
	virtual void InternalDeactivate();

	DECLARE_EVENT_HANDLER(Travel, OnTravel);

public:
};

}

#endif