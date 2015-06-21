#include "MocapClip.h"

#include <Animation/NodeControllerMocap.h>
#include <Core/Factory.h>

namespace Anim
{
__ImplementClass(Anim::CMocapClip, 'MCLP', Anim::CAnimClip);

bool CMocapClip::Setup(const CArray<CMocapTrack>& _Tracks, const CArray<CStrID>& TrackMapping,
					   const CArray<CEventTrack>* _EventTracks, vector4* _pKeys,
					   DWORD _KeysPerCurve, DWORD _KeyStride, float _KeyTime)
{
	n_assert(_pKeys);

//	if (State == Resources::Rsrc_Loaded) Unload();

	pKeys = _pKeys;
	Tracks = _Tracks;
	if (_EventTracks) EventTracks = *_EventTracks;
	else EventTracks.Clear();

	KeysPerCurve = _KeysPerCurve;
	KeyStride = _KeyStride;
	InvKeyTime = 1.f / _KeyTime;
	Duration = (KeysPerCurve - 1) * _KeyTime;

	for (int i = 0; i < Tracks.GetCount(); ++i)
	{
		Tracks[i].pOwnerClip = this;
		CSampler& Sampler = Samplers.GetOrAdd(TrackMapping[i]);
		switch (Tracks[i].Channel)
		{
			case Scene::Tfm_Translation:	Sampler.pTrackT = &Tracks[i]; break;
			case Scene::Tfm_Rotation:		Sampler.pTrackR = &Tracks[i]; break;
			case Scene::Tfm_Scaling:		Sampler.pTrackS = &Tracks[i]; break;
			default: Sys::Error("CMocapClip::Setup() -> Unsupported channel for an SRT sampler track!");
		};
	}

	OK;
}
//---------------------------------------------------------------------

void CMocapClip::Unload()
{
	//!!!sampler pointers will become invalid! use smartptrs?
	// or in controller store clip ptr and clear sampler if clip becomes unloaded

	Samplers.Clear();
	Tracks.Clear();
	SAFE_DELETE_ARRAY(pKeys);
}
//---------------------------------------------------------------------

Scene::PNodeController CMocapClip::CreateController(DWORD SamplerIdx) const
{
	Anim::PNodeControllerMocap Ctlr = n_new(Anim::CNodeControllerMocap);
	Ctlr->SetSampler(&Samplers.ValueAt(SamplerIdx));
	return Ctlr.GetUnsafe();
}
//---------------------------------------------------------------------

}
