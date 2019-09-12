#include "NodeControllerMocap.h"

#include <Animation/MocapTrack.h>

namespace Anim
{

void CNodeControllerMocap::SetSampler(const CSampler* _pSampler)
{
	n_assert(_pSampler); //???allow nullptr?
	pSampler = _pSampler;
	Channels.ClearAll();
	if (pSampler->pTrackT) Channels.Set(Scene::Tfm_Translation);
	if (pSampler->pTrackR) Channels.Set(Scene::Tfm_Rotation);
	if (pSampler->pTrackS) Channels.Set(Scene::Tfm_Scaling);
}
//---------------------------------------------------------------------

void CNodeControllerMocap::Clear()
{
	Channels.ClearAll();
	pSampler = nullptr;
	KeyIndex = INVALID_INDEX;
	IpolFactor = 0.f;
}
//---------------------------------------------------------------------

bool CNodeControllerMocap::ApplyTo(Math::CTransformSRT& DestTfm)
{
	if (!pSampler || (!pSampler->pTrackT && !pSampler->pTrackR && !pSampler->pTrackS)) FAIL;

	if (pSampler->pTrackT) ((CMocapTrack*)pSampler->pTrackT)->Sample(KeyIndex, IpolFactor, DestTfm.Translation);
	if (pSampler->pTrackR) ((CMocapTrack*)pSampler->pTrackR)->Sample(KeyIndex, IpolFactor, DestTfm.Rotation);
	if (pSampler->pTrackS) ((CMocapTrack*)pSampler->pTrackS)->Sample(KeyIndex, IpolFactor, DestTfm.Scale);

	OK;
}
//---------------------------------------------------------------------

}
