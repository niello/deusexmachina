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
FACTORY_CLASS_IMPL(Frame::CTerrainAttribute, 'TRNA', Frame::CRenderableAttribute);

bool CTerrainAttribute::LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count)
{
	for (UPTR j = 0; j < Count; ++j)
	{
		const uint32_t Code = DataReader.Read<uint32_t>();
		switch (Code)
		{
			case 'CDLD':
			{
				_CDLODDataUID = DataReader.Read<CStrID>();
				_HeightMapUID = CStrID(_CDLODDataUID.CStr() + CString("#HM"));
				break;
			}
			case 'MTRL':
			{
				_MaterialUID = DataReader.Read<CStrID>();
				break;
			}
			case 'TSSX':
			{
				_InvSplatSizeX = 1.f / DataReader.Read<float>();
				break;
			}
			case 'TSSZ':
			{
				_InvSplatSizeZ = 1.f / DataReader.Read<float>();
				break;
			}
			default: FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CTerrainAttribute::Clone()
{
	PTerrainAttribute ClonedAttr = n_new(CTerrainAttribute());
	ClonedAttr->_MaterialUID = _MaterialUID;
	ClonedAttr->_CDLODDataUID = _CDLODDataUID;
	ClonedAttr->_HeightMapUID = _HeightMapUID;
	ClonedAttr->_InvSplatSizeX = _InvSplatSizeX;
	ClonedAttr->_InvSplatSizeZ = _InvSplatSizeZ;
	ClonedAttr->_CDLODData = _CDLODData;
	return ClonedAttr;
}
//---------------------------------------------------------------------

bool CTerrainAttribute::ValidateResources(Resources::CResourceManager& ResMgr)
{
	auto RCDLODData = ResMgr.RegisterResource<Render::CCDLODData>(_CDLODDataUID);
	_CDLODData = RCDLODData->ValidateObject<Render::CCDLODData>();
	OK;
}
//---------------------------------------------------------------------

bool CTerrainAttribute::ValidateGPUResources(CGraphicsResourceManager& ResMgr)
{
	if (!ResMgr.GetGPU()) FAIL;

	// HeightMap support check
	//!!!write R32F variant!
	if (!ResMgr.GetGPU()->CheckCaps(Render::Caps_VSTex_R16)) FAIL;

	if (!_CDLODData)
	{
		n_assert2(false, "CTerrainAttribute::ValidateGPUResources() > ValidateResources must be called before this!");
		if (!ValidateResources(*ResMgr.GetResourceManager()) || !_CDLODData) FAIL;
	}

	if (!Renderable) Renderable.reset(n_new(Render::CTerrain()));
	auto pTerrain = static_cast<Render::CTerrain*>(Renderable.get());

	const auto PatchSize = _CDLODData->GetPatchSize();
	if (!IsPow2(PatchSize) || PatchSize < 4) FAIL;

	// TODO: don't copy, store once!
	pTerrain->InvSplatSizeX = _InvSplatSizeX;
	pTerrain->InvSplatSizeZ = _InvSplatSizeZ;
	pTerrain->CDLODData = _CDLODData;

	pTerrain->Material = _MaterialUID ? ResMgr.GetMaterial(_MaterialUID) : nullptr;
	pTerrain->HeightMap = _HeightMapUID ? ResMgr.GetTexture(_HeightMapUID, Render::Access_GPU_Read) : nullptr;

	CString PatchName;
	PatchName.Format("#Mesh_Patch%dx%d", PatchSize, PatchSize);
	CStrID MeshUID(PatchName);
	if (!ResMgr.GetResourceManager()->FindResource(MeshUID))
		ResMgr.GetResourceManager()->RegisterResource(MeshUID, n_new(Resources::CMeshGeneratorQuadPatch(PatchSize)));

	pTerrain->PatchMesh = ResMgr.GetMesh(MeshUID);

	const auto QPatchSize = (PatchSize >> 1);
	PatchName.Format("#Mesh_Patch%dx%d", QPatchSize, QPatchSize);
	CStrID QuarterMeshUID(PatchName);
	if (!ResMgr.GetResourceManager()->FindResource(QuarterMeshUID))
		ResMgr.GetResourceManager()->RegisterResource(QuarterMeshUID, n_new(Resources::CMeshGeneratorQuadPatch(QPatchSize)));

	pTerrain->QuarterPatchMesh = ResMgr.GetMesh(QuarterMeshUID);

	OK;
}
//---------------------------------------------------------------------

bool CTerrainAttribute::GetLocalAABB(CAABB& OutBox, UPTR LOD) const
{
	if (!_CDLODData) FAIL;
	OutBox = _CDLODData->GetAABB();
	OK;
}
//---------------------------------------------------------------------

}