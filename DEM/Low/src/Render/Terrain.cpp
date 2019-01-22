#include "Terrain.h"

#include <Render/Material.h>
#include <Render/MeshGenerators.h>
#include <Render/GPUDriver.h>
#include <Render/CDLODData.h>
#include <Render/Mesh.h>
#include <Render/Texture.h>
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
			CString RUID("Terrain:");
			RUID += DataReader.Read<CStrID>().CStr();
			RUID += ".cdlod";
			RCDLODData = ResourceMgr->RegisterResource<Render::CCDLODData>(CStrID(RUID));
			HeightMapUID = CStrID(RUID + "#HM");
			OK;
		}
		case 'MTRL':
		{
			CString RsrcID = DataReader.Read<CString>();
			MaterialUID = CStrID(CString("Materials:") + RsrcID.CStr() + ".mtl"); //???replace ID by full URI on export?
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
	pCloned->MaterialUID = MaterialUID;
	pCloned->HeightMapUID = HeightMapUID;
	pCloned->PatchMesh = PatchMesh;
	pCloned->QuarterPatchMesh = QuarterPatchMesh;
	pCloned->InvSplatSizeX = InvSplatSizeX;
	pCloned->InvSplatSizeZ = InvSplatSizeZ;
	return pCloned;
}
//---------------------------------------------------------------------

bool CTerrain::GetLocalAABB(CAABB& OutBox, UPTR LOD) const
{
	OutBox = CDLODData->GetAABB();
	OK;
}
//---------------------------------------------------------------------

bool CTerrain::ValidateResources(CGPUDriver* pGPU)
{
	// HeightMap support check
	//!!!write R32F variant!
	if (!pGPU->CheckCaps(Render::Caps_VSTex_R16)) FAIL;

	CDLODData = RCDLODData->ValidateObject<Render::CCDLODData>();

	//!!!if CDLOD will not include texture, just height data, create texture here, if not created!
	//can create CDLOD textures here per GPU with fixed sub-ID, so with no unnecessary recreation

	Material = pGPU ? pGPU->GetMaterial(MaterialUID) : nullptr;
	HeightMap = pGPU ? pGPU->GetTexture(HeightMapUID, Render::Access_GPU_Read) : nullptr;

	U32 PatchSize = CDLODData->GetPatchSize();
	if (pGPU && IsPow2(PatchSize) && PatchSize >= 4)
	{
		CString PatchName;
		PatchName.Format("#Mesh_Patch%dx%d", PatchSize, PatchSize);
		CStrID MeshUID(PatchName);
		Resources::PResource RPatch = ResourceMgr->FindResource(MeshUID);
		if (!RPatch)
		{
			Resources::PMeshGeneratorQuadPatch GenQuad = n_new(Resources::CMeshGeneratorQuadPatch);
			GenQuad->GPU = pGPU;
			GenQuad->QuadsPerEdge = PatchSize;
			ResourceMgr->RegisterResource(MeshUID, GenQuad);
		}
		PatchMesh = RPatch->ValidateObject<CMesh>();

		PatchSize >>= 1;
		PatchName.Format("#Mesh_Patch%dx%d", PatchSize, PatchSize);
		CStrID QuarterMeshUID(PatchName);
		RPatch = ResourceMgr->FindResource(QuarterMeshUID);
		if (!RPatch)
		{
			Resources::PMeshGeneratorQuadPatch GenQuad = n_new(Resources::CMeshGeneratorQuadPatch);
			GenQuad->GPU = pGPU;
			GenQuad->QuadsPerEdge = PatchSize;
			ResourceMgr->RegisterResource(QuarterMeshUID, GenQuad);
		}
		QuarterPatchMesh = RPatch->ValidateObject<CMesh>();
	}

	OK;
}
//---------------------------------------------------------------------

}