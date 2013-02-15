#pragma once
#ifndef __DEM_L1_ANIM_CLIP_H__
#define __DEM_L1_ANIM_CLIP_H__

#include <Resources/Resource.h>
#include <Animation/AnimTrack.h>

// Animation clip is a set of tracks (curves), which defines single animation for a set of points
// in space. Typically it consists of up to 3 tracks * number of animated bones in target sceleton.
// Clip can have any number of tracks and target bones, so it can animate a single scene node as well.

namespace Anim
{

//!!!can subclass CAnimClipKeyframed & CAnimClipMocap! allow the first to load both formats

class CAnimClip: public Resources::CResource
{
protected:

	// bone&channel-to-track mapping (to curve track indices)
	float					Duration;
	//bool					Looping;
	nFixedArray<CAnimTrack>	Tracks;

public:

	CAnimClip(CStrID ID, Resources::IResourceManager* pHost): CResource(ID, pHost) {}

	bool				Setup();
	virtual void		Unload();
};

typedef Ptr<CMesh> PMesh;

}

#endif
