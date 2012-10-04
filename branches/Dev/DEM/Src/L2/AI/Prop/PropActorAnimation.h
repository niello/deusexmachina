#pragma once
#ifndef __DEM_L2_PROP_ACTOR_ANIM_H_
#define __DEM_L2_PROP_ACTOR_ANIM_H_

#include <game/property.h>

// Control the animations of a actor.
// Based on mangalore ActorAnimationProperty (C) 2005 Radon Labs GmbH

namespace Properties
{

using namespace Events;

class CPropActorAnimation: public Game::CProperty
{
	DeclareRTTI;
	DeclareFactory(CPropActorAnimation);
	DeclarePropertyStorage;

protected:

	void RequestAnimation(const nString& BaseAnimation, const nString& OverlayAnimation, float BaseAnimTimeOffset) const;

	DECLARE_EVENT_HANDLER(ActorStopMoving, OnActorStopMoving);
	//DECLARE_2_EVENTS_HANDLER(ActorMoveDir, ActorSetSpeed, OnMoveDirOrSpeed);

public:

	virtual void	Activate();
	virtual void	Deactivate();
};

RegisterFactory(CPropActorAnimation);

}

#endif
