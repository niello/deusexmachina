#include "AmbientLightAttribute.h"
#include <Frame/GraphicsResourceManager.h>
#include <Scene/SPS.h>
#include <Render/GPUDriver.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Frame
{
FACTORY_CLASS_IMPL(Frame::CAmbientLightAttribute, 'NAAL', Scene::CNodeAttribute);

bool CAmbientLightAttribute::LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count)
{
	for (UPTR j = 0; j < Count; ++j)
	{
		const uint32_t Code = DataReader.Read<uint32_t>();
		switch (Code)
		{
			case 'IRRM':
			{
				IrradianceMapUID = CStrID(DataReader.Read<CString>());
				break;
			}
			case 'PMRM':
			{
				RadianceEnvMapUID = CStrID(DataReader.Read<CString>());
				break;
			}
			default: FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool CAmbientLightAttribute::ValidateGPUResources(CGraphicsResourceManager& ResMgr)
{
	IrradianceMap = ResMgr.GetTexture(IrradianceMapUID, Render::Access_GPU_Read);
	RadianceEnvMap = ResMgr.GetTexture(RadianceEnvMapUID, Render::Access_GPU_Read);
	OK;
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CAmbientLightAttribute::Clone()
{
	PAmbientLightAttribute ClonedAttr = n_new(CAmbientLightAttribute());
	ClonedAttr->IrradianceMapUID = IrradianceMapUID;
	ClonedAttr->RadianceEnvMapUID = RadianceEnvMapUID;
	ClonedAttr->IrradianceMap = IrradianceMap;
	ClonedAttr->RadianceEnvMap = RadianceEnvMap;
	return ClonedAttr;
}
//---------------------------------------------------------------------

void CAmbientLightAttribute::OnActivityChanged(bool Active)
{
	if (!Active && pSPS)
	{
		if (pSPSRecord)
		{
			pSPS->RemoveRecord(pSPSRecord);
			pSPSRecord = nullptr;
		}
		else pSPS->OversizedObjects.RemoveByValue(this);

		pSPS = nullptr;
	}
}
//---------------------------------------------------------------------

void CAmbientLightAttribute::UpdateInSPS(Scene::CSPS& SPS)
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
bool CAmbientLightAttribute::GetGlobalAABB(CAABB& OutBox) const
{
	NOT_IMPLEMENTED;
	FAIL;
}
//---------------------------------------------------------------------

}