#include "NodeAttrAmbientLight.h"
#include <Frame/FrameResourceManager.h>
#include <Scene/SPS.h>
#include <Render/GPUDriver.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Frame
{
__ImplementClass(Frame::CNodeAttrAmbientLight, 'NAAL', Scene::CNodeAttribute);

bool CNodeAttrAmbientLight::LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count)
{
	for (UPTR j = 0; j < Count; ++j)
	{
		const uint32_t Code = DataReader.Read<uint32_t>();
		switch (Code)
		{
			case 'IRRM':
			{
				UIDIrradianceMap = CStrID(DataReader.Read<CString>());
				break;
			}
			case 'PMRM':
			{
				UIDRadianceEnvMap = CStrID(DataReader.Read<CString>());
				break;
			}
			default: FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool CNodeAttrAmbientLight::ValidateResources(CFrameResourceManager& ResMgr)
{
	IrradianceMap = ResMgr.GetTexture(UIDIrradianceMap, Render::Access_GPU_Read);
	RadianceEnvMap = ResMgr.GetTexture(UIDRadianceEnvMap, Render::Access_GPU_Read);
	OK;
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CNodeAttrAmbientLight::Clone()
{
	PNodeAttrAmbientLight ClonedAttr = n_new(CNodeAttrAmbientLight);
	ClonedAttr->UIDIrradianceMap = UIDIrradianceMap;
	ClonedAttr->UIDRadianceEnvMap = UIDRadianceEnvMap;
	ClonedAttr->IrradianceMap = IrradianceMap;
	ClonedAttr->RadianceEnvMap = RadianceEnvMap;
	return ClonedAttr.Get();
}
//---------------------------------------------------------------------

void CNodeAttrAmbientLight::OnDetachFromScene()
{
	//???do it on deactivation of an attribute? even it is not detached from node
	if (pSPS)
	{
		if (pSPSRecord)
		{
			pSPS->RemoveRecord(pSPSRecord);
			pSPSRecord = nullptr;
		}
		else pSPS->OversizedObjects.RemoveByValue(this);

		pSPS = nullptr;
	}

	CNodeAttribute::OnDetachFromScene();
}
//---------------------------------------------------------------------

void CNodeAttrAmbientLight::UpdateInSPS(Scene::CSPS& SPS)
{
	const bool Global = true;
	if (Global)
	{
		if (pSPSRecord)
		{
			pSPS->RemoveRecord(pSPSRecord);
			pSPSRecord = nullptr;
		}

		if (!pSPS)
		{
			pSPS = &SPS;
			SPS.OversizedObjects.Add(this);
		}
	}
	else
	{
		NOT_IMPLEMENTED;
		//???need per-frame? leave only add & remove? env maps are static
		/*
		if (pSPS != &SPS)
		{
			if (pSPS)
			{
				if (pSPSRecord)
				{
					pSPS->RemoveRecord(pSPSRecord);
					pSPSRecord = nullptr;
				}
				else pSPS->OversizedObjects.RemoveByValue(this);
			}

			pSPS = &SPS;
		}
		
		if (!pSPSRecord)
		{
			CAABB Box;
			GetGlobalAABB(Box); //???calc cached and reuse here?
			pSPSRecord = SPS.AddRecord(Box, this);
			Flags.Clear(WorldMatrixChanged);
		}
		else if (Flags.Is(WorldMatrixChanged))
		{
			GetGlobalAABB(pSPSRecord->GlobalBox);
			SPS.UpdateRecord(pSPSRecord);
			Flags.Clear(WorldMatrixChanged);
		}
		*/
	}
}
//---------------------------------------------------------------------

//!!!GetGlobalAABB & CalcBox must be separate!
bool CNodeAttrAmbientLight::GetGlobalAABB(CAABB& OutBox) const
{
	NOT_IMPLEMENTED;
	FAIL;
}
//---------------------------------------------------------------------

}