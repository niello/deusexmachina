#pragma once
#ifndef __DEM_L1_ANIM_KEYFRAME_CLIP_H__
#define __DEM_L1_ANIM_KEYFRAME_CLIP_H__

#include <Animation/AnimClip.h>
#include <Animation/KeyframeTrack.h>
#include <Data/Dictionary.h>

// This clip stores keyframe data per-track and, unlike the CMocapClip, has no specific optimizations.
// Advantage of this clip is its flexibility and lower memory consumption.

namespace Anim
{

class CKeyframeClip: public CAnimClip
{
	__DeclareClass(CKeyframeClip);

protected:

	nArray<CKeyframeTrack>	Tracks;		//???use fixed array?

public:

	CKeyframeClip(CStrID ID): CAnimClip(ID) {}
	virtual ~CKeyframeClip() { if (IsLoaded()) Unload(); }

	bool							Setup(	const nArray<CKeyframeTrack>& _Tracks, const nArray<CStrID>& TrackMapping,
											const nArray<CEventTrack>* _EventTracks, float Length);
	virtual void					Unload();

	virtual Scene::PNodeController	CreateController(DWORD SamplerIdx) const;
};

typedef Ptr<CKeyframeClip> PKeyframeClip;

}

#endif
