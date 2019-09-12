#include "NodeControllerKeyframe.h"

#include <Animation/KeyframeTrack.h>

namespace Anim
{

void CNodeControllerKeyframe::SetSampler(const CSampler* _pSampler)
{
	n_assert(_pSampler); //???allow nullptr?
	pSampler = _pSampler;
	Channels.ClearAll();
	if (pSampler->pTrackT) Channels.Set(Scene::Tfm_Translation);
	if (pSampler->pTrackR) Channels.Set(Scene::Tfm_Rotation);
	if (pSampler->pTrackS) Channels.Set(Scene::Tfm_Scaling);
}
//---------------------------------------------------------------------

void CNodeControllerKeyframe::Clear()
{
	Channels.ClearAll();
	pSampler = nullptr;
	Time = 0.f;
}
//---------------------------------------------------------------------

bool CNodeControllerKeyframe::ApplyTo(Math::CTransformSRT& DestTfm)
{
	if (!pSampler || (!pSampler->pTrackT && !pSampler->pTrackR && !pSampler->pTrackS)) FAIL;

	if (pSampler->pTrackT) ((CKeyframeTrack*)pSampler->pTrackT)->Sample(Time, DestTfm.Translation);
	if (pSampler->pTrackR) ((CKeyframeTrack*)pSampler->pTrackR)->Sample(Time, DestTfm.Rotation);
	if (pSampler->pTrackS) ((CKeyframeTrack*)pSampler->pTrackS)->Sample(Time, DestTfm.Scale);

	OK;
}
//---------------------------------------------------------------------

}
