#pragma once
#ifndef __DEM_L1_ANIM_CTLR_MOCAP_H__
#define __DEM_L1_ANIM_CTLR_MOCAP_H__

#include <Scene/NodeController.h>
#include <Animation/AnimFwd.h>

// Animation controller, that samples transform from mocap clip tracks.
// It involves mocap-specific optimization of calculating keyframes from
// time outside the controllers once per mocap clip sampling.

namespace Anim
{

class CNodeControllerMocap: public Scene::CNodeController
{
protected:

	//???store clip ptr? sampler of invalid clip will cause crash! sampler ptr can be used as a cache
	const CSampler*		pSampler;
	int					KeyIndex;
	float				IpolFactor;

public:

	CNodeControllerMocap(): pSampler(nullptr), KeyIndex(INVALID_INDEX), IpolFactor(0.f) { Flags.Set(LocalSpace); }

	void			SetSampler(const CSampler* _pSampler);
	void			SetSamplingParams(int KeyIdx, float Factor) { KeyIndex = KeyIdx; IpolFactor = Factor; }
	void			Clear();
	virtual bool	ApplyTo(Math::CTransformSRT& DestTfm);
};

typedef Ptr<CNodeControllerMocap> PNodeControllerMocap;

}

#endif
