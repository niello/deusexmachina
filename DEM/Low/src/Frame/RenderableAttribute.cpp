#include "RenderableAttribute.h"
#include <Scene/SPS.h>
#include <Scene/SceneNode.h>
#include <Render/Renderable.h>

namespace Frame
{
RTTI_CLASS_IMPL(Frame::CRenderableAttribute, Scene::CNodeAttribute);

void CRenderableAttribute::UpdateInSPS(Scene::CSPS& SPS)
{
	if (!Renderable) return;

	CAABB AABB;
	const bool AABBIsValid = Renderable->GetLocalAABB(AABB);
	const bool SPSChanged = (pSPS != &SPS);

	// Remove record, if AABB is invalid or object is moved to another SPS (one scene - one SPS for now)
	if (pSPS && (SPSChanged || !AABBIsValid))
	{
		if (pSPSRecord)
		{
			pSPS->RemoveRecord(pSPSRecord);
			pSPSRecord = nullptr;
		}
		else pSPS->OversizedObjects.RemoveByValue(this);
	}

	// If AABB is valid, add/update SPS record
	if (AABBIsValid)
	{
		pSPS = &SPS;

		if (AABB.IsEmpty())
		{
			if (SPSChanged) SPS.OversizedObjects.Add(this);
		}
		else
		{
			if (!pSPSRecord)
			{
				AABB.Transform(_pNode->GetWorldMatrix());
				pSPSRecord = SPS.AddRecord(AABB, this);
			}
			else if (_pNode->GetTransformVersion() != LastTransformVersion) //!!! || LocalBox changed!
			{
				AABB.Transform(_pNode->GetWorldMatrix(), pSPSRecord->GlobalBox);
				SPS.UpdateRecord(pSPSRecord);
			}

			LastTransformVersion = _pNode->GetTransformVersion();
		}
	}
	else pSPS = nullptr;
}
//---------------------------------------------------------------------

bool CRenderableAttribute::GetGlobalAABB(CAABB& OutBox, UPTR LOD) const
{
	if (!_pNode) FAIL;

	if (pSPSRecord && _pNode->GetTransformVersion() == LastTransformVersion) //!!! && LocalBox not changed!
	{
		OutBox = pSPSRecord->GlobalBox;
	}
	else
	{
		if (!Renderable || !Renderable->GetLocalAABB(OutBox, LOD)) FAIL;
		OutBox.Transform(_pNode->GetWorldMatrix());
	}

	OK;
}
//---------------------------------------------------------------------

void CRenderableAttribute::OnActivityChanged(bool Active)
{
	if (!Active && pSPS)
	{
		if (pSPSRecord)
		{
			pSPS->RemoveRecord(pSPSRecord);
			pSPSRecord = nullptr;
		}
		else
		{
			pSPS->OversizedObjects.RemoveByValue(this);
		}
		pSPS = nullptr;
	}
}
//---------------------------------------------------------------------

}