#include "TerrainAttribute.h"
#include <Frame/GraphicsResourceManager.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <Render/GPUDriver.h>
#include <Render/MeshGenerators.h>
#include <Render/Terrain.h>
#include <Render/CDLODData.h>
#include <Render/Mesh.h>
#include <Render/Material.h>
#include <Render/Texture.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Frame
{
__ImplementClass(Frame::CTerrainAttribute, 'TRNA', Frame::CRenderableAttribute);

bool CTerrainAttribute::LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count)
{
	if (!Renderable) Renderable.reset(n_new(Render::CTerrain));
	auto pTerrain = static_cast<Render::CTerrain*>(Renderable.get());

	for (UPTR j = 0; j < Count; ++j)
	{
		const uint32_t Code = DataReader.Read<uint32_t>();
		switch (Code)
		{
			case 'CDLD':
			{
				CDLODDataUID = DataReader.Read<CStrID>();
				HeightMapUID = CStrID(CDLODDataUID.CStr() + CString("#HM"));
				break;
			}
			case 'MTRL':
			{
				MaterialUID = DataReader.Read<CStrID>();
				break;
			}
			case 'TSSX':
			{
				pTerrain->InvSplatSizeX = 1.f / DataReader.Read<float>();
				break;
			}
			case 'TSSZ':
			{
				pTerrain->InvSplatSizeZ = 1.f / DataReader.Read<float>();
				break;
			}
			default: FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool CTerrainAttribute::ValidateResources(Resources::CResourceManager& ResMgr)
{
	if (!Renderable) Renderable.reset(n_new(Render::CTerrain));
	auto pTerrain = static_cast<Render::CTerrain*>(Renderable.get());

	auto RCDLODData = ResMgr.RegisterResource<Render::CCDLODData>(CDLODDataUID);
	pTerrain->CDLODData = RCDLODData->ValidateObject<Render::CCDLODData>();

	OK;
}
//---------------------------------------------------------------------

bool CTerrainAttribute::ValidateGPUResources(CGraphicsResourceManager& ResMgr)
{
	if (!ResMgr.GetGPU()) FAIL;

	// HeightMap support check
	//!!!write R32F variant!
	if (!ResMgr.GetGPU()->CheckCaps(Render::Caps_VSTex_R16)) FAIL;

	if (!Renderable) Renderable.reset(n_new(Render::CTerrain));
	auto pTerrain = static_cast<Render::CTerrain*>(Renderable.get());

	if (!pTerrain->CDLODData)
	{
		n_assert2(false, "CTerrainAttribute::ValidateGPUResources() > ValidateResources must be called before this!");
		if (!ValidateResources(*ResMgr.GetResourceManager())) FAIL;
	}

	pTerrain->Material = MaterialUID ? ResMgr.GetMaterial(MaterialUID) : nullptr;
	pTerrain->HeightMap = HeightMapUID ? ResMgr.GetTexture(HeightMapUID, Render::Access_GPU_Read) : nullptr;

	U32 PatchSize = pTerrain->CDLODData->GetPatchSize();
	if (IsPow2(PatchSize) && PatchSize >= 4)
	{
		CString PatchName;
		PatchName.Format("#Mesh_Patch%dx%d", PatchSize, PatchSize);

		//!!!double searching! Here and in GPU->CreateMesh!
		CStrID MeshUID(PatchName);
		if (!ResMgr.GetResourceManager()->FindResource(MeshUID))
		{
			ResMgr.GetResourceManager()->RegisterResource(MeshUID, n_new(Resources::CMeshGeneratorQuadPatch(PatchSize)));
		}

		pTerrain->PatchMesh = ResMgr.GetMesh(MeshUID);

		PatchSize >>= 1;
		PatchName.Format("#Mesh_Patch%dx%d", PatchSize, PatchSize);
		CStrID QuarterMeshUID(PatchName);
		if (!ResMgr.GetResourceManager()->FindResource(QuarterMeshUID))
		{
			ResMgr.GetResourceManager()->RegisterResource(QuarterMeshUID, n_new(Resources::CMeshGeneratorQuadPatch(PatchSize)));
		}

		pTerrain->QuarterPatchMesh = ResMgr.GetMesh(QuarterMeshUID);
	}

	OK;
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CTerrainAttribute::Clone()
{
	PTerrainAttribute ClonedAttr = n_new(CTerrainAttribute);
	if (Renderable) ClonedAttr->Renderable = std::move(Renderable->Clone());
	ClonedAttr->MaterialUID = MaterialUID;
	ClonedAttr->CDLODDataUID = CDLODDataUID;
	ClonedAttr->HeightMapUID = HeightMapUID;
	return ClonedAttr.Get();
}
//---------------------------------------------------------------------

}