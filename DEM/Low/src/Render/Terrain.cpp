#include "Terrain.h"

#include <Render/Material.h>
#include <Render/MeshGenerators.h>
#include <Render/GPUDriver.h>
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
#include <Resources/ResourceCreator.h>
#include <IO/BinaryReader.h>
#include <IO/PathUtils.h>
#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CTerrain, 'TERR', Render::IRenderable);

CTerrain::CTerrain() {}
CTerrain::~CTerrain() {}

bool CTerrain::LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader)
{
	switch (FourCC.Code)
	{
		case 'CDLD':
		{
			CString RsrcURI("Terrain:");
			RsrcURI += DataReader.Read<CStrID>().CStr();
			RsrcURI += ".cdlod";
			RCDLODData = ResourceMgr->RegisterResource(RsrcURI);
			OK;
		}
		case 'MTRL':
		{
			CString RsrcID = DataReader.Read<CString>();
			CStrID RsrcURI = CStrID(CString("Materials:") + RsrcID.CStr() + ".mtl"); //???replace ID by full URI on export?
			RMaterial = ResourceMgr->RegisterResource(RsrcURI);
			OK;
		}
		case 'TSSX':
		{
			InvSplatSizeX = 1.f / DataReader.Read<float>();
			OK;
		}
		case 'TSSZ':
		{
			InvSplatSizeZ = 1.f / DataReader.Read<float>();
			OK;
		}
		default: FAIL;
	}
}
//---------------------------------------------------------------------

IRenderable* CTerrain::Clone()
{
	CTerrain* pCloned = n_new(CTerrain);
	pCloned->RCDLODData = RCDLODData;
	pCloned->RMaterial = RMaterial;
	pCloned->PatchMesh = PatchMesh;
	pCloned->QuarterPatchMesh = QuarterPatchMesh;
	pCloned->InvSplatSizeX = InvSplatSizeX;
	pCloned->InvSplatSizeZ = InvSplatSizeZ;
	return pCloned;
}
//---------------------------------------------------------------------

bool CTerrain::ValidateResources(CGPUDriver* pGPU)
{
	if (!RCDLODData->IsLoaded())
	{
		Resources::PResourceLoader Loader = RCDLODData->GetLoader();
		if (Loader.IsNullPtr())
			Loader = ResourceMgr->CreateDefaultLoaderFor<Render::CCDLODData>(PathUtils::GetExtension(RCDLODData->GetUID()));
		ResourceMgr->LoadResourceSync(*RCDLODData, *Loader);
		if (!RCDLODData->IsLoaded()) FAIL;
	}
	CDLODData = RCDLODData->GetObject<Render::CCDLODData>();

	//!!!if CDLOD will not include texture, just height data, create texture here, if not created!

	if (!RMaterial->IsLoaded())
	{
		Resources::PResourceLoader Loader = RMaterial->GetLoader();
		if (Loader.IsNullPtr())
			Loader = ResourceMgr->CreateDefaultLoaderFor<Render::CMaterial>(PathUtils::GetExtension(RMaterial->GetUID()));
		ResourceMgr->LoadResourceSync(*RMaterial, *Loader);
		n_assert(RMaterial->IsLoaded());
	}
	Material = RMaterial->GetObject<Render::CMaterial>();

	U32 PatchSize = CDLODData->GetPatchSize();
	if (pGPU && IsPow2(PatchSize) && PatchSize >= 4)
	{
		CString PatchName;
		PatchName.Format("Mesh_Patch%dx%d", PatchSize, PatchSize);
		Resources::PResource RPatch = ResourceMgr->RegisterResource(PatchName.CStr());
		if (!RPatch->IsLoaded())
		{
			Resources::PResourceCreator Gen = RPatch->GetGenerator();
			if (Gen.IsNullPtr())
			{
				Resources::PMeshGeneratorQuadPatch GenQuad = n_new(Resources::CMeshGeneratorQuadPatch);
				GenQuad->GPU = pGPU;
				GenQuad->QuadsPerEdge = PatchSize;
				Gen = GenQuad.Get();
			}
			ResourceMgr->GenerateResourceSync(*RPatch, *Gen);
			n_assert(RPatch->IsLoaded());
		}
		PatchMesh = RPatch->GetObject<CMesh>();

		PatchSize >>= 1;
		PatchName.Format("Mesh_Patch%dx%d", PatchSize, PatchSize);
		RPatch = ResourceMgr->RegisterResource(PatchName.CStr());
		if (!RPatch->IsLoaded())
		{
			Resources::PResourceCreator Gen = RPatch->GetGenerator();
			if (Gen.IsNullPtr())
			{
				Resources::PMeshGeneratorQuadPatch GenQuad = n_new(Resources::CMeshGeneratorQuadPatch);
				GenQuad->GPU = pGPU;
				GenQuad->QuadsPerEdge = PatchSize;
				Gen = GenQuad.Get();
			}
			ResourceMgr->GenerateResourceSync(*RPatch, *Gen);
			n_assert(RPatch->IsLoaded());
		}
		QuarterPatchMesh = RPatch->GetObject<CMesh>();
	}

	OK;
}
//---------------------------------------------------------------------

}