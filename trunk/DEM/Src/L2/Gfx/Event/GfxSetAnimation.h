#pragma once
#ifndef __DEM_L2_NEVENT_GFX_SET_ANIM_H__
#define __DEM_L2_NEVENT_GFX_SET_ANIM_H__

#include <Events/EventNative.h>
#include <Gfx/CharEntity.h>

// Set base or overlay animation on an actor.

namespace Event
{

class GfxSetAnimation: public Events::CEventNative
{
    DeclareRTTI;
    DeclareFactory(GfxSetAnimation);

public:

	nTime	FadeInTime;
	nString	BaseAnim;
	nString	OverlayAnim;
	nTime	BaseAnimTimeOffset;
	nTime	OverlayAnimDurationOverride;
	bool	StopOverlayAnim;

	nArray<nString>	MixedBaseAnimNames;
	nArray<float>	MixedBaseAnimWeights;
	nArray<nString>	MixedOverlayAnimNames;
	nArray<float>	MixedOverlayAnimWeights;

	GfxSetAnimation();
};

RegisterFactory(GfxSetAnimation);

inline GfxSetAnimation::GfxSetAnimation() :
	FadeInTime(0.2),
	BaseAnimTimeOffset(0.0),
	OverlayAnimDurationOverride(0.0),
	StopOverlayAnim(false)
{
}
//---------------------------------------------------------------------

}

#endif
