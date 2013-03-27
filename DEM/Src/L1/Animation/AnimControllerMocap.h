#pragma once
#ifndef __DEM_L1_ANIM_CTLR_MOCAP_H__
#define __DEM_L1_ANIM_CTLR_MOCAP_H__

#include <Scene/AnimController.h>
#include <Animation/MocapClip.h>

// Animation controller, that samples transform from mocap clip tracks.
// It involves mocap-specific optimization of calculating keyframes from
// time outside the controllers once per mocap clip sampling.

namespace Anim
{

class CAnimControllerMocap: public Scene::CAnimController
{
protected:

	//???store clip ptr? sampler of invalid clip will cause crash! sampler ptr can be used as a cache
	const CMocapClip::CSampler*	pSampler;
	int							KeyIndex;
	float						IpolFactor;

public:

	CAnimControllerMocap(): pSampler(NULL), KeyIndex(INVALID_INDEX), IpolFactor(0.f) {}

	void			SetSampler(const CMocapClip::CSampler* _pSampler);
	void			SetSamplingState(int KeyIdx, float Factor) { KeyIndex = KeyIdx; IpolFactor = Factor; }
	void			Clear();
	virtual bool	ApplyTo(Math::CTransformSRT& DestTfm);
};

typedef Ptr<CAnimControllerMocap> PAnimControllerClip;

}

#endif
