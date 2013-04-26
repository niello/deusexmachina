#pragma once
#ifndef __DEM_L1_ANIM_CLIP_H__
#define __DEM_L1_ANIM_CLIP_H__

#include <Resources/Resource.h>
#include <Animation/Anim.h>
#include <util/ndictionary.h>

// Animation clip is a set of tracks (curves), which defines single animation for a set of points
// in space. Typically it consists of up to 3 tracks * number of animated bones in target sceleton.
// Clip can have any number of tracks and target bones, so it can animate a single scene node as well.
// Tracks are grouped in samplers. One sampler affects one target, referencing it by a node relative
// path or by a bone ID.

//!!!TODO: unify MCA & KFA mapping. Map to node relative path, CPropAnimation can cache CStrID with the path -> Node.

namespace Scene
{
	class CSceneNode;
	typedef Ptr<class CAnimController> PAnimController;
}

namespace Anim
{

class CAnimClip: public Resources::CResource
{
	DeclareRTTI;

protected:

	CSamplerList	Samplers;
	float			Duration;

public:

	CAnimClip(CStrID ID): CResource(ID) {}

	virtual Scene::PAnimController	CreateController(DWORD SamplerIdx) const = 0;
	float							AdjustTime(float Time, bool Loop) const;
	DWORD							GetSamplerCount() const { return Samplers.Size(); }
	CBoneID							GetSamplerTarget(DWORD Idx) const { return Samplers.KeyAtIndex(Idx); }
	float							GetDuration() const { return Duration; }
};

typedef Ptr<CAnimClip> PAnimClip;

inline float CAnimClip::AdjustTime(float Time, bool Loop) const
{
	if (Loop)
	{
		Time = n_fmod(Time, Duration);
		if (Time < 0.f) Time += Duration;
	}
	else
	{
		if (Time < 0.f) Time = 0.f;
		else if (Time > Duration) Time = Duration;
	}

	return Time;
}
//---------------------------------------------------------------------

}

#endif
