#include "MocapClip.h"

namespace Anim
{

bool CMocapClip::Setup(const nArray<CMocapTrack>& _Tracks, const nArray<int>& TrackMapping, vector4* _pKeys)
{
	n_assert(_pKeys);

	if (State == Resources::Rsrc_Loaded) Unload();

	pKeys = _pKeys;
	Tracks = _Tracks;

	InvKeyTime = (float)(KeysPerCurve - 1) / Duration;

	for (int i = 0; i < Tracks.Size(); ++i)
	{
		Tracks[i].pOwnerClip = this;
		CSampler& Sampler = Samplers.GetOrAdd(TrackMapping[i]);
		switch (Tracks[i].Channel)
		{
			case Chnl_Translation:	Sampler.pTrackT = &Tracks[i]; break;
			case Chnl_Rotation:		Sampler.pTrackR = &Tracks[i]; break;
			case Chnl_Scaling:		Sampler.pTrackS = &Tracks[i]; break;
			default: n_error("CMocapClip::Setup() -> Unsupported channel for an SRT sampler track!");
		};
	}

	State = Resources::Rsrc_Loaded;
	OK;
}
//---------------------------------------------------------------------

void CMocapClip::Unload()
{
	State = Resources::Rsrc_NotLoaded;

	//!!!sampler pointers will become invalid! use smartptrs?
	// or in controller store clip ptr and clear sampler if clip becomes unloaded

	Samplers.Clear();
	Tracks.Clear();
	SAFE_DELETE_ARRAY(pKeys);
}
//---------------------------------------------------------------------

}
