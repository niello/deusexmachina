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

bool CTerrain::LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count)
{
	for (UPTR j = 0; j < Count; ++j)
	{
		const uint32_t Code = DataReader.Read<uint32_t>();
		switch (Code)
		{
			case 'CDLD':
			{
				CString RUID("Terrain:");
				RUID += DataReader.Read<CStrID>().CStr();
				RUID += ".cdlod";
				RCDLODData = ResourceMgr->RegisterResource<Render::CCDLODData>(CStrID(RUID));
				HeightMapUID = CStrID(RUID + "#HM");
				break;
			}
			case 'MTRL':
			{
				CString RsrcID = DataReader.Read<CString>();
				MaterialUID = CStrID(CString("Materials:") + RsrcID.CStr() + ".mtl"); //???replace ID by full URI on export?
				break;
			}
			case 'TSSX':
			{
				InvSplatSizeX = 1.f / DataReader.Read<float>();
				break;
			}
			case 'TSSZ':
			{
				InvSplatSizeZ = 1.f / DataReader.Read<float>();
				break;
			}
			default: FAIL;
		}
	}

	OK;
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

		//!!!double searching! Here and in GPU->CreateMesh!
		CStrID MeshUID(PatchName);
		if (!ResourceMgr->FindResource(MeshUID))
		{
			ResourceMgr->RegisterResource(MeshUID, n_new(Resources::CMeshGeneratorQuadPatch(PatchSize)));
		}

		PatchMesh = pGPU ? pGPU->GetMesh(MeshUID) : nullptr;

		PatchSize >>= 1;
		PatchName.Format("#Mesh_Patch%dx%d", PatchSize, PatchSize);
		CStrID QuarterMeshUID(PatchName);
		if (!ResourceMgr->FindResource(QuarterMeshUID))
		{
			ResourceMgr->RegisterResource(QuarterMeshUID, n_new(Resources::CMeshGeneratorQuadPatch(PatchSize)));
		}

		QuarterPatchMesh = pGPU ? pGPU->GetMesh(QuarterMeshUID) : nullptr;
	}

	OK;
}
//---------------------------------------------------------------------

}