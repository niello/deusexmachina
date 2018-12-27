#pragma once
#ifndef __DEM_L1_ANIM_CTLR_KEYFRAME_H__
#define __DEM_L1_ANIM_CTLR_KEYFRAME_H__

#include <Scene/NodeController.h>
#include <Animation/AnimFwd.h>

// Animation controller, that samples transform from simple keyframed clip tracks.
// It performs all calculations in a per-track manner.

namespace Anim
{

class CNodeControllerKeyframe: public Scene::CNodeController
{
protected:

	//???store clip ptr? sampler of invalid clip will cause crash! sampler ptr can be used as a cache
	const CSampler*	pSampler;
	float			Time;

public:

	CNodeControllerKeyframe(): pSampler(NULL), Time(0.f) { Flags.Set(LocalSpace); }

	void			SetSampler(const CSampler* _pSampler);
	void			SetTime(float AbsTime) { Time = AbsTime; }
	void			Clear();
	virtual bool	ApplyTo(Math::CTransformSRT& DestTfm);
};

typedef Ptr<CNodeControllerKeyframe> PNodeControllerKeyframe;

}

#endif
