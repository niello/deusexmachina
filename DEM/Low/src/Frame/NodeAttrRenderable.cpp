#include "NodeAttrRenderable.h"
#include <Scene/SPS.h>
#include <Scene/SceneNode.h>
#include <Render/Renderable.h>

namespace Frame
{
__ImplementClassNoFactory(Frame::CNodeAttrRenderable, Scene::CNodeAttribute);

void CNodeAttrRenderable::UpdateInSPS(Scene::CSPS& SPS)
{
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
		else if (!pSPSRecord)
		{
			AABB.Transform(pNode->GetWorldMatrix());
			pSPSRecord = SPS.AddRecord(AABB, this);
		}
		else if (Flags.Is(WorldMatrixChanged)) //!!! || LocalBox changed!
		{
			AABB.Transform(pNode->GetWorldMatrix(), pSPSRecord->GlobalBox);
			SPS.UpdateRecord(pSPSRecord);
		}
	}
	else pSPS = nullptr;

	Flags.Clear(WorldMatrixChanged);
}
//---------------------------------------------------------------------

bool CNodeAttrRenderable::GetGlobalAABB(CAABB& OutBox, UPTR LOD) const
{
	if (!pNode) FAIL;

	if (pSPSRecord && Flags.IsNot(WorldMatrixChanged)) //!!! && LocalBox not changed!
	{
		OutBox = pSPSRecord->GlobalBox;
	}
	else
	{
		if (!Renderable || !Renderable->GetLocalAABB(OutBox, LOD)) FAIL;
		OutBox.Transform(pNode->GetWorldMatrix());
	}

	OK;
}
//---------------------------------------------------------------------

void CNodeAttrRenderable::OnDetachFromScene()
{
//	//???do it on deactivation of an attribute? even it is not detached from node
	if (pSPSRecord)
	{
		pSPS->RemoveRecord(pSPSRecord);
		pSPSRecord = nullptr;
		pSPS = nullptr;
	}
	else if (pSPS)
	{
		pSPS->OversizedObjects.RemoveByValue(this);
		pSPS = nullptr;
	}

	CNodeAttribute::OnDetachFromScene();
}
//---------------------------------------------------------------------

}