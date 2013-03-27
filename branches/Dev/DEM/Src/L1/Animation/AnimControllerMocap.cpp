#include "AnimControllerMocap.h"

namespace Anim
{

void CAnimControllerMocap::SetSampler(const CMocapClip::CSampler* _pSampler)
{
	n_assert(_pSampler); //???allow NULL?
	pSampler = _pSampler;
	Channels.ClearAll();
	if (pSampler->pTrackT) Channels.Set(Chnl_Translation);
	if (pSampler->pTrackR) Channels.Set(Chnl_Rotation);
	if (pSampler->pTrackS) Channels.Set(Chnl_Scaling);
}
//---------------------------------------------------------------------

void CAnimControllerMocap::Clear()
{
	Channels.ClearAll();
	pSampler = NULL;
	KeyIndex = INVALID_INDEX;
	IpolFactor = 0.f;
}
//---------------------------------------------------------------------

bool CAnimControllerMocap::ApplyTo(Math::CTransformSRT& DestTfm)
{
	if (!pSampler) FAIL;

	if (pSampler->pTrackT) pSampler->pTrackT->Sample(KeyIndex, IpolFactor, DestTfm.Translation);
	if (pSampler->pTrackR) pSampler->pTrackR->Sample(KeyIndex, IpolFactor, DestTfm.Rotation);
	if (pSampler->pTrackS) pSampler->pTrackS->Sample(KeyIndex, IpolFactor, DestTfm.Scale);

	OK;
}
//---------------------------------------------------------------------

}
