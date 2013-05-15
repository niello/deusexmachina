#pragma once
#ifndef __DEM_L3_PROP_TRANSITION_ZONE_H__
#define __DEM_L3_PROP_TRANSITION_ZONE_H__

#include <Game/Property.h>
#include <DB/AttrID.h>

// Transition zone allows characters (both player-controlled & NPC) to travel between locations.
// Provides data for IAO Travel action.

//???need property or can provide only attributes?
//Prop can add IAO name like "Travel to <Level Name>"

namespace Attr
{
	DeclareString(TargetLevelID);	// ID of the target level
	DeclareString(DestPoint);		// Destination point, ID of any transformable entity in the target level
};

namespace Properties
{

class CPropTransitionZone: public Game::CProperty
{
	__DeclareClass(CPropTransitionZone);
	__DeclarePropertyStorage;

protected:

	DECLARE_EVENT_HANDLER(Travel, OnTravel);

public:

	virtual void	Activate();
	virtual void	Deactivate();
	virtual void	GetAttributes(nArray<DB::CAttrID>& Attrs);
};

__RegisterClassInFactory(CPropTransitionZone);

}

#endif