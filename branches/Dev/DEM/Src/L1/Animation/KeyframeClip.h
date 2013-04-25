#pragma once
#ifndef __DEM_L1_ANIM_KEYFRAME_CLIP_H__
#define __DEM_L1_ANIM_KEYFRAME_CLIP_H__

#include <Animation/AnimClip.h>
#include <Animation/KeyframeTrack.h>
#include <Data/SimpleString.h>
#include <util/ndictionary.h>

// This clip stores keyframe data per-track and, unlike the CMocapClip, has no specific optimizations.
// Advantage of this clip is its flexibility and lower memory consumption.

namespace Anim
{

class CKeyframeClip: public CAnimClip
{
	DeclareRTTI;

protected:

	nArray<CKeyframeTrack>	Tracks;		//???use fixed array?

public:

	CKeyframeClip(CStrID ID): CAnimClip(ID) {}

	bool							Setup(const nArray<CKeyframeTrack>& _Tracks, const nArray<Data::CSimpleString>& _TrackMapping, float Length);
	virtual void					Unload();

	virtual Scene::PAnimController	CreateController(DWORD SamplerIdx) const;
};

typedef Ptr<CKeyframeClip> PKeyframeClip;

}

#endif
