#include "NodeAttrRenderable.h"

#include <Scene/SPS.h>
#include <Scene/SceneNode.h>
#include <Render/Renderable.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Frame
{
__ImplementClass(Frame::CNodeAttrRenderable, 'NARE', Scene::CNodeAttribute);

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

void CNodeAttrRenderable::UpdateInSPS(Scene::CSPS& SPS)
{
	// Moved to another SPS (one scene - one SPS for now)
	bool SPSChanged = (pSPS != &SPS);
	if (SPSChanged)
	{
		if (pSPS)
		{
			if (pSPSRecord)
			{
				pSPS->RemoveRecord(pSPSRecord);
				pSPSRecord = NULL;
			}
			else pSPS->OversizedObjects.RemoveByValue(this);
		}

		pSPS = &SPS;
	}

	CAABB GlobalAABB;
	if (pRenderable->GetLocalAABB(GlobalAABB))
	{
		if (!pSPSRecord)
		{
			GlobalAABB.Transform(pNode->GetWorldMatrix());
			pSPSRecord = SPS.AddRecord(GlobalAABB, this);
		}
		else if (Flags.Is(WorldMatrixChanged)) //!!! || LocalBox changed!
		{
			GlobalAABB.Transform(pNode->GetWorldMatrix(), pSPSRecord->GlobalBox);
			SPS.UpdateRecord(pSPSRecord);
		}
	}
	else
	{
		if (SPSChanged) SPS.OversizedObjects.Add(this);
	}

	Flags.Clear(WorldMatrixChanged);
}
//---------------------------------------------------------------------

bool CNodeAttrRenderable::GetGlobalAABB(CAABB& OutBox, UPTR LOD) const
{
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

void CNodeAttrRenderable::OnDetachFromNode()
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

	CNodeAttribute::OnDetachFromNode();
}
//---------------------------------------------------------------------

}