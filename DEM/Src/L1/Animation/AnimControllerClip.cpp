#include "AnimControllerClip.h"

namespace Anim
{

void CAnimControllerClip::SetTrack(CMocapTrack* pTrack, EChannel Channel)
{
	n_assert_dbg(pTrack->Channel == Channel);
	if (pTrack) Channels.Set(Channel);
	else Channels.Clear(Channel);
	switch (Channel)
	{
		case Chnl_Translation:	pTrackT = pTrack; break;
		case Chnl_Rotation:		pTrackR = pTrack; break;
		case Chnl_Scaling:		pTrackS = pTrack; break;
		default: n_error("CAnimControllerClip::SetTrack() -> Unsupported channel!");
	};
}
//---------------------------------------------------------------------

void CAnimControllerClip::Clear()
{
	Channels.ClearAll();
	pTrackT = NULL;
	pTrackR = NULL;
	pTrackS = NULL;
	CurrTime = 0.f;
}
//---------------------------------------------------------------------

bool CAnimControllerClip::ApplyTo(Math::CTransformSRT& DestTfm)
{
	if (!pTrackT && !pTrackR && !pTrackS) FAIL;

	//???if (Channels.Is(Translation))?
	if (pTrackT) pTrackT->Sample(CurrTime, DestTfm.Translation);
	if (pTrackR) pTrackR->Sample(CurrTime, DestTfm.Rotation);
	if (pTrackS) pTrackS->Sample(CurrTime, DestTfm.Scale);

	OK;
}
//---------------------------------------------------------------------

}
