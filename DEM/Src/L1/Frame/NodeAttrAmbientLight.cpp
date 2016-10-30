#include "NodeAttrAmbientLight.h"

#include <Scene/SPS.h>
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <IO/PathUtils.h>
#include <Core/Factory.h>

namespace Frame
{
__ImplementClass(Frame::CNodeAttrAmbientLight, 'NAAL', Scene::CNodeAttribute);

bool CNodeAttrAmbientLight::LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader)
{
	switch (FourCC.Code)
	{
		case 'IRRM':
		{
			CString RsrcURI = DataReader.Read<CString>();
			Resources::PResource RTexture = ResourceMgr->RegisterResource(RsrcURI);			
			if (!RTexture->IsLoaded())
			{
				Resources::PResourceLoader Loader = RTexture->GetLoader();
				if (Loader.IsNullPtr())
					Loader = ResourceMgr->CreateDefaultLoaderFor<Render::CTexture>(PathUtils::GetExtension(RsrcURI));
				ResourceMgr->LoadResourceSync(*RTexture, *Loader);
				if (!RTexture->IsLoaded()) FAIL;
			}
			IrradianceMap = RTexture->GetObject<Render::CTexture>();
			OK;
		}
		case 'PMRM':
		{
			CString RsrcURI = DataReader.Read<CString>();
			Resources::PResource RTexture = ResourceMgr->RegisterResource(RsrcURI);			
			if (!RTexture->IsLoaded())
			{
				Resources::PResourceLoader Loader = RTexture->GetLoader();
				if (Loader.IsNullPtr())
					Loader = ResourceMgr->CreateDefaultLoaderFor<Render::CTexture>(PathUtils::GetExtension(RsrcURI));
				ResourceMgr->LoadResourceSync(*RTexture, *Loader);
				if (!RTexture->IsLoaded()) FAIL;
			}
			RadianceEnvMap = RTexture->GetObject<Render::CTexture>();
			OK;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CNodeAttrAmbientLight::Clone()
{
	PNodeAttrAmbientLight ClonedAttr = n_new(CNodeAttrAmbientLight);
	ClonedAttr->IrradianceMap = IrradianceMap;
	ClonedAttr->RadianceEnvMap = RadianceEnvMap;
	return ClonedAttr.GetUnsafe();
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
			pSPSRecord = NULL;
		}
		else pSPS->OversizedObjects.RemoveByValue(this);

		pSPS = NULL;
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
			pSPSRecord = NULL;
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
					pSPSRecord = NULL;
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