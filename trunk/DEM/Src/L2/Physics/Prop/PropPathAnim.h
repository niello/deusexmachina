#pragma once
#ifndef __DEM_L2_PROP_PATH_ANIM_H__
#define __DEM_L2_PROP_PATH_ANIM_H__

#include <game/property.h>
#include <db/AttrID.h>
#include <anim2/nanimation.h>

// Attach this property to an entity to move the entity along an
// animation path. Take care that the property won't collide
// with other properties which influence an entity's position.
// Based on mangalore PathAnimProperty_ (C) 2005 Radon Labs GmbH

namespace Attr
{
	DeclareString(AnimPath);        // filename of animation file
	DeclareBool(AnimRelative);      // animate absolute position or from current position
	DeclareBool(AnimLoop);          // loop or clamp animations
	DeclareBool(AnimPlaying);       // true if animation currently playing
};

namespace Properties
{

using namespace Events;

class CPropPathAnim: public Game::CProperty
{
	DeclareRTTI;
	DeclareFactory(CPropPathAnim);
	DeclarePropertyStorage;
	DeclarePropertyPools(Game::LivePool);

protected:

	nRef<nAnimation>	refAnimation;
	matrix44			InitialMatrix;
	nTime				AnimTime;

	DECLARE_EVENT_HANDLER(PathAnimPlay, OnPathAnimPlay);
	DECLARE_EVENT_HANDLER(PathAnimStop, OnPathAnimStop);
	DECLARE_EVENT_HANDLER(PathAnimRewind, OnPathAnimRewind);

	DECLARE_EVENT_HANDLER(OnMoveBefore, OnMoveBefore);

	void UpdateAnimation();

public:

	CPropPathAnim();
	virtual ~CPropPathAnim();

	virtual void GetAttributes(nArray<DB::CAttrID>& Attrs);
	virtual void Activate();
	virtual void Deactivate();
};

RegisterFactory(CPropPathAnim);

}

#endif
