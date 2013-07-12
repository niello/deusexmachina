#pragma once
#ifndef __DEM_L1_ANIM_CLIP_H__
#define __DEM_L1_ANIM_CLIP_H__

#include <Resources/Resource.h>
#include <Animation/EventTrack.h>
#include <Data/Dictionary.h>

// Animation clip is a set of tracks (curves), which defines single animation for a set of points
// in space. Typically it consists of up to 3 tracks * number of animated bones in target sceleton.
// Clip can have any number of tracks and target bones, so it can animate a single scene node as well.
// Tracks are grouped in samplers. One sampler affects one node, referencing it by a relative path.
// There are also event tracks that fire events when reach specified points on the timeline.

namespace Data
{
	typedef Ptr<class CParams> PParams;
}

namespace Events
{
	class CEventDispatcher;
}

namespace Scene
{
	class CSceneNode;
	typedef Ptr<class CNodeController> PNodeController;
}

namespace Anim
{

class CAnimClip: public Resources::CResource
{
	__DeclareClassNoFactory;

protected:

	CDict<CStrID, CSampler>	Samplers;
	nArray<CEventTrack>				EventTracks; //???use fixed array? //???per-sampler event tracks (are 3D editors capable)?
	float							Duration;

public:

	CAnimClip(CStrID ID): CResource(ID) {}

	virtual Scene::PNodeController	CreateController(DWORD SamplerIdx) const = 0;
	float							AdjustTime(float Time, bool Loop) const;
	void							FireEvents(float ExactTime, bool Loop, Events::CEventDispatcher* pDisp = NULL, Data::PParams Params = NULL) const;
	void							FireEvents(float StartTime, float EndTime, bool Loop, Events::CEventDispatcher* pDisp = NULL, Data::PParams Params = NULL) const;
	DWORD							GetSamplerCount() const { return Samplers.GetCount(); }
	CStrID							GetSamplerTarget(DWORD Idx) const { return Samplers.KeyAt(Idx); }
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
