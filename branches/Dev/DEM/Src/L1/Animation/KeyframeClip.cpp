#include "KeyframeClip.h"

#include <Animation/NodeControllerKeyframe.h>
#include <Core/Factory.h>

namespace Anim
{
__ImplementClass(Anim::CKeyframeClip, 'KCLP', Anim::CAnimClip);

bool CKeyframeClip::Setup(const CArray<CKeyframeTrack>& _Tracks, const CArray<CStrID>& TrackMapping,
						  const CArray<CEventTrack>* _EventTracks, float Length)
{
//	if (State == Resources::Rsrc_Loaded) Unload();

	Tracks = _Tracks;
	if (_EventTracks) EventTracks = *_EventTracks;
	else EventTracks.Clear();

	Duration = Length;

	for (int i = 0; i < Tracks.GetCount(); ++i)
	{
		CSampler& Sampler = Samplers.GetOrAdd(TrackMapping[i]);
		switch (Tracks[i].Channel)
		{
			case Scene::Tfm_Translation:	Sampler.pTrackT = &Tracks[i]; break;
			case Scene::Tfm_Rotation:		Sampler.pTrackR = &Tracks[i]; break;
			case Scene::Tfm_Scaling:		Sampler.pTrackS = &Tracks[i]; break;
			default: Sys::Error("CKeyframeClip::Setup() -> Unsupported channel for an SRT sampler track!");
		};
	}

	OK;
}
//---------------------------------------------------------------------

void CKeyframeClip::Unload()
{
	//!!!sampler pointers will become invalid! use smartptrs?
	// or in controller store clip ptr and clear sampler if clip becomes unloaded

	Samplers.Clear();
	Tracks.Clear();
}
//---------------------------------------------------------------------

Scene::PNodeController CKeyframeClip::CreateController(DWORD SamplerIdx) const
{
	Anim::PNodeControllerKeyframe Ctlr = n_new(Anim::CNodeControllerKeyframe);
	Ctlr->SetSampler(&Samplers.ValueAt(SamplerIdx));
	return Ctlr.GetUnsafe();
}
//---------------------------------------------------------------------

}
