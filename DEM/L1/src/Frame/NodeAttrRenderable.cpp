#include "NodeAttrRenderable.h"

#include <Scene/SPS.h>
#include <Scene/SceneNode.h>
#include <Render/Renderable.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Frame
{
__ImplementClass(Frame::CNodeAttrRenderable, 'NARE', Scene::CNodeAttribute);

CNodeAttrRenderable::~CNodeAttrRenderable()
{
	SAFE_DELETE(pRenderable);
}
//---------------------------------------------------------------------

bool CNodeAttrRenderable::LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader)
{
	switch (FourCC.Code)
	{
		case 'RCLS':
		{
			//???!!!load by fourcc?!
			n_assert_dbg(!pRenderable);
			CString ClassName = DataReader.Read<CString>();
			pRenderable = (Render::IRenderable*)Factory->Create(ClassName);
			OK;
		}
		default: return pRenderable && pRenderable->LoadDataBlock(FourCC, DataReader);
	}
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CNodeAttrRenderable::Clone()
{
	//???or clone renderable?
	PNodeAttrRenderable ClonedAttr = n_new(CNodeAttrRenderable);
	ClonedAttr->pRenderable = pRenderable->Clone();
	return ClonedAttr.GetUnsafe();
}
//---------------------------------------------------------------------

void CNodeAttrRenderable::UpdateInSPS(Scene::CSPS& SPS)
{
	CAABB AABB;
	const bool AABBIsValid = pRenderable->GetLocalAABB(AABB);
	const bool SPSChanged = (pSPS != &SPS);

	// Remove record, if AABB is invalid or object is moved to another SPS (one scene - one SPS for now)
	if (pSPS && (SPSChanged || !AABBIsValid))
	{
		if (pSPSRecord)
		{
			pSPS->RemoveRecord(pSPSRecord);
			pSPSRecord = NULL;
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
	else pSPS = NULL;

	Flags.Clear(WorldMatrixChanged);
}
//---------------------------------------------------------------------

bool CNodeAttrRenderable::GetGlobalAABB(CAABB& OutBox, UPTR LOD) const
{
	n_assert_dbg(pNode);
	if (pSPSRecord && Flags.IsNot(WorldMatrixChanged)) //!!! && LocalBox not changed!
	{
		OutBox = pSPSRecord->GlobalBox;
		OK;
	}
	else
	{
		if (!pRenderable->GetLocalAABB(OutBox, LOD)) FAIL;
		OutBox.Transform(pNode->GetWorldMatrix());
		OK;
	}
}
//---------------------------------------------------------------------

void CNodeAttrRenderable::OnDetachFromScene()
{
//	//???do it on deactivation of an attribute? even it is not detached from node
	if (pSPSRecord)
	{
		pSPS->RemoveRecord(pSPSRecord);
		pSPSRecord = NULL;
		pSPS = NULL;
	}
	else if (pSPS)
	{
		pSPS->OversizedObjects.RemoveByValue(this);
		pSPS = NULL;
	}

	CNodeAttribute::OnDetachFromScene();
}
//---------------------------------------------------------------------

}