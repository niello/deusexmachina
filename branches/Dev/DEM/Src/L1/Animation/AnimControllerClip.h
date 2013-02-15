#pragma once
#ifndef __DEM_L1_ANIM_CTLR_CLIP_H__
#define __DEM_L1_ANIM_CTLR_CLIP_H__

#include <Scene/AnimController.h>
#include <Animation/MocapTrack.h> //!!!Now mocap only!

// Animation controller, that samples transform from animation clip tracks.

namespace Anim
{

class CAnimControllerClip: public Scene::CAnimController
{
protected:

	CMocapTrack*	pTrackT;
	CMocapTrack*	pTrackR;
	CMocapTrack*	pTrackS;
	float			CurrTime;

public:

	CAnimControllerClip(): pTrackT(NULL), pTrackR(NULL), pTrackS(NULL), CurrTime(0.f) {}

	void			SetTrack(CMocapTrack* pTrack, EChannel Channel);
	void			SetTime(float Time) { CurrTime = Time; }
	void			Clear();
	virtual bool	ApplyTo(Math::CTransformSRT& DestTfm);
};

typedef Ptr<CAnimControllerClip> PAnimControllerClip;

}

#endif
