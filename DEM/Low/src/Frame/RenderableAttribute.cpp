#include "RenderableAttribute.h"
#include <Scene/SPS.h>
#include <Scene/SceneNode.h>
#include <Debug/DebugDraw.h>

namespace Frame
{

void CRenderableAttribute::UpdateInSPS(Scene::CSPS& SPS)
{
	n_assert_dbg(IsActive());

	CAABB AABB;
	const bool AABBIsValid = GetLocalAABB(AABB);
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
		if (GetLocalAABB(OutBox, LOD)) FAIL;
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

void CRenderableAttribute::RenderDebug(Debug::CDebugDraw& DebugDraw) const
{
	CAABB AABB;
	if (GetGlobalAABB(AABB))
		DebugDraw.DrawBoxWireframe(AABB, Render::ColorRGBA(160, 220, 255, 255), 1.f);

	if (pSPS && pSPSRecord)
		DebugDraw.DrawBoxWireframe(pSPS->GetNodeAABB(pSPSRecord->NodeIndex, true), Render::ColorRGBA(160, 255, 160, 255), 1.f);
}
//---------------------------------------------------------------------

}
