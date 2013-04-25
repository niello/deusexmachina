#include "AnimControllerKeyframe.h"

#include <Animation/KeyframeTrack.h>

namespace Anim
{

void CAnimControllerKeyframe::SetSampler(const CSampler* _pSampler)
{
	n_assert(_pSampler); //???allow NULL?
	pSampler = _pSampler;
	Channels.ClearAll();
	if (pSampler->pTrackT) Channels.Set(Chnl_Translation);
	if (pSampler->pTrackR) Channels.Set(Chnl_Rotation);
	if (pSampler->pTrackS) Channels.Set(Chnl_Scaling);
}
//---------------------------------------------------------------------

void CAnimControllerKeyframe::Clear()
{
	Channels.ClearAll();
	pSampler = NULL;
	Time = 0.f;
}
//---------------------------------------------------------------------

bool CAnimControllerKeyframe::ApplyTo(Math::CTransformSRT& DestTfm)
{
	if (!pSampler || (!pSampler->pTrackT && !pSampler->pTrackR && !pSampler->pTrackS)) FAIL;

	if (pSampler->pTrackT) ((CKeyframeTrack*)pSampler->pTrackT)->Sample(Time, DestTfm.Translation);
	if (pSampler->pTrackR) ((CKeyframeTrack*)pSampler->pTrackR)->Sample(Time, DestTfm.Rotation);
	if (pSampler->pTrackS) ((CKeyframeTrack*)pSampler->pTrackS)->Sample(Time, DestTfm.Scale);

	OK;
}
//---------------------------------------------------------------------

}
