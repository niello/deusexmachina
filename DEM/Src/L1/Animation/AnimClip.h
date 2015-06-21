#pragma once
#ifndef __DEM_L1_ANIM_CLIP_H__
#define __DEM_L1_ANIM_CLIP_H__

#include <Resources/ResourceObject.h>
#include <Animation/EventTrack.h>
#include <Data/Dictionary.h>

// Animation clip is a set of tracks (curves), which defines single animation for a set of points
// in space. Typically it consists of up to 3 tracks * number of animated bones in target skeleton.
// Clip can have any number of tracks and target bones, so it can animate a single scene node as well.
// Tracks are grouped in samplers. One sampler affects one node, referencing it by a relative path.
// There are also event tracks that fire events when reach specified points on the timeline.
// NB: TIMELINE scale corresponds the real TIME only when Speed = 1 (forward direction, no speed scaling).
// To emphasize this difference, clip timeline is referenced as 'scale' or 'timeline', some point on that
// scale - as 'animation cursor position', and duration of its segment - as 'length'.

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
	typedef Ptr<class CNodeController> PNodeController;
}

namespace Anim
{

class CAnimClip: public Resources::CResourceObject
{
	__DeclareClassNoFactory;

protected:

	CDict<CStrID, CSampler>	Samplers;
	CArray<CEventTrack>		EventTracks; //???use fixed array? //???per-sampler event tracks (are 3D editors capable)?
	float					Duration;

public:

	virtual Scene::PNodeController	CreateController(DWORD SamplerIdx) const = 0;

	virtual bool					IsResourceValid() const { FAIL; }

	float	AdjustCursorPos(float Pos, bool Loop) const;
	void	FireEvents(float ExactCursorPos, bool Loop, Events::CEventDispatcher* pDisp = NULL, Data::PParams Params = NULL) const;
	void	FireEvents(float StartCursorPos, float EndCursorPos, bool Loop, Events::CEventDispatcher* pDisp = NULL, Data::PParams Params = NULL) const;
	DWORD	GetSamplerCount() const { return Samplers.GetCount(); }
	CStrID	GetSamplerTarget(DWORD Idx) const { return Samplers.KeyAt(Idx); }
	float	GetDuration() const { return Duration; }
};

typedef Ptr<CAnimClip> PAnimClip;

inline float CAnimClip::AdjustCursorPos(float Pos, bool Loop) const
{
	if (Loop)
	{
		Pos = n_fmod(Pos, Duration);
		if (Pos < 0.f) Pos += Duration;
	}
	else
	{
		if (Pos < 0.f) Pos = 0.f;
		else if (Pos > Duration) Pos = Duration;
	}

	return Pos;
}
//---------------------------------------------------------------------

}

#endif
