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

	CArray<CKeyframeTrack>	Tracks;		//???use fixed array?

public:

	virtual ~CKeyframeClip() { Unload(); }

	bool							Setup(	const CArray<CKeyframeTrack>& _Tracks, const CArray<CStrID>& TrackMapping,
											const CArray<CEventTrack>* _EventTracks, float Length);
	virtual void					Unload();

	virtual bool					IsResourceValid() const { return !!Tracks.GetCount(); }

	virtual Scene::PNodeController	CreateController(UPTR SamplerIdx) const;
};

typedef Ptr<CKeyframeClip> PKeyframeClip;

}

#endif
