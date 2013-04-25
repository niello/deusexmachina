#pragma once
#ifndef __DEM_L1_ANIM_CTLR_KEYFRAME_H__
#define __DEM_L1_ANIM_CTLR_KEYFRAME_H__

#include <Scene/AnimController.h>

// Animation controller, that samples transform from simple keyframed clip tracks.
// It performs all calculations in a per-track manner.

namespace Anim
{

class CAnimControllerKeyframe: public Scene::CAnimController
{
protected:

	//???store clip ptr? sampler of invalid clip will cause crash! sampler ptr can be used as a cache
	const CSampler*	pSampler;
	float			Time;

public:

	CAnimControllerKeyframe(): pSampler(NULL), Time(0.f) { Flags.Set(LocalSpace); }

	void			SetSampler(const CSampler* _pSampler);
	void			SetTime(float AbsTime) { Time = AbsTime; }
	void			Clear();
	virtual bool	ApplyTo(Math::CTransformSRT& DestTfm);
};

typedef Ptr<CAnimControllerKeyframe> PAnimControllerKeyframe;

}

#endif
