#include "TerrainAttribute.h"
#include <Frame/View.h>
#include <Frame/GraphicsResourceManager.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <Render/GPUDriver.h>
#include <Render/MeshGenerators.h>
#include <Render/Terrain.h>
#include <Render/CDLODData.h>
#include <Render/Mesh.h>
#include <Render/Material.h>
#include <Render/Effect.h>
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
	if (!_CDLODData)
	{
		auto RCDLODData = ResMgr.RegisterResource<Render::CCDLODData>(_CDLODDataUID.CStr());
		_CDLODData = RCDLODData->ValidateObject<Render::CCDLODData>();
	}
	OK;
}
//---------------------------------------------------------------------

Render::PRenderable CTerrainAttribute::CreateRenderable() const
{
	return std::make_unique<Render::CTerrain>();
}
//---------------------------------------------------------------------

void CTerrainAttribute::UpdateRenderable(CView& View, Render::IRenderable& Renderable) const
{
	auto& ResMgr = *View.GetGraphicsManager();

	// HeightMap support check
	//!!!write R32F variant!
	if (!ResMgr.GetGPU() || !ResMgr.GetGPU()->CheckCaps(Render::Caps_VSTex_R16)) return;

	auto pTerrain = static_cast<Render::CTerrain*>(&Renderable);

	pTerrain->InvSplatSizeX = _InvSplatSizeX;
	pTerrain->InvSplatSizeZ = _InvSplatSizeZ;

	// Setup heightmap texture
	if (!_HeightMapUID)
		pTerrain->HeightMap = nullptr;
	if (!pTerrain->HeightMap) //!!!FIXME: store UID inside and check changes as with other gfx resources?!
		pTerrain->HeightMap = ResMgr.GetTexture(_HeightMapUID, Render::Access_GPU_Read);

	// Initialize material
	if (!_MaterialUID)
	{
		if (pTerrain->Material)
		{
			pTerrain->Material = nullptr;
			pTerrain->ShaderTechIndex = INVALID_INDEX_T<U32>;
			pTerrain->RenderQueueMask = 0;
			pTerrain->MaterialKey = 0;
			pTerrain->ShaderTechKey = 0;
		}
	}
	else if (!pTerrain->Material || pTerrain->Material->GetUID() != _MaterialUID) //!!! || LOD != RememberedLOD? Or terrain manages its material LOD in a renderer?
	{
		static const CStrID InputSet_CDLOD("CDLOD");

		pTerrain->Material = View.GetGraphicsManager()->GetMaterial(_MaterialUID);
		if (pTerrain->Material && pTerrain->Material->GetEffect())
		{
			pTerrain->ShaderTechIndex = View.RegisterEffect(*pTerrain->Material->GetEffect(), InputSet_CDLOD);
			pTerrain->RenderQueueMask = (1 << pTerrain->Material->GetEffect()->GetType());
			pTerrain->MaterialKey = pTerrain->Material->GetSortingKey();
			pTerrain->ShaderTechKey = View.GetShaderTechCache()[pTerrain->ShaderTechIndex]->GetSortingKey(); //???FIXME: now we use non-overridden tech key for all phases
		}
	}

	// Initialize CDLOD data and meshes
	if (!_CDLODData)
	{
		pTerrain->CDLODData = nullptr;
		pTerrain->PatchMesh = nullptr;
		pTerrain->QuarterPatchMesh = nullptr;
		pTerrain->GeometryKey = 0;
	}
	else if (!pTerrain->CDLODData || pTerrain->CDLODData != _CDLODData)
	{
		pTerrain->CDLODData = _CDLODData;

		const auto PatchSize = _CDLODData->GetPatchSize();
		//if (!Math::IsPow2(PatchSize) || PatchSize < 4) FAIL;

		//!!!TODO: store both patches in one mesh, only use different primitive groups (can vary only indices!)
		CString PatchName;
		PatchName.Format("#Mesh_Patch%dx%d", PatchSize, PatchSize);
		CStrID MeshUID(PatchName);
		if (!ResMgr.GetResourceManager()->FindResource(MeshUID))
			ResMgr.GetResourceManager()->RegisterResource(MeshUID.CStr(), n_new(Resources::CMeshGeneratorQuadPatch(PatchSize)));

		pTerrain->PatchMesh = ResMgr.GetMesh(MeshUID);

		const auto QPatchSize = (PatchSize >> 1);
		PatchName.Format("#Mesh_Patch%dx%d", QPatchSize, QPatchSize);
		CStrID QuarterMeshUID(PatchName);
		if (!ResMgr.GetResourceManager()->FindResource(QuarterMeshUID))
			ResMgr.GetResourceManager()->RegisterResource(QuarterMeshUID.CStr(), n_new(Resources::CMeshGeneratorQuadPatch(QPatchSize)));

		pTerrain->QuarterPatchMesh = ResMgr.GetMesh(QuarterMeshUID);

		pTerrain->GeometryKey = pTerrain->PatchMesh->GetSortingKey();
	}

	//!!!TODO: if camera or heightmap or transform changed, update visible patches!
}
//---------------------------------------------------------------------

void CTerrainAttribute::UpdateLightList(Render::IRenderable& Renderable, const CObjectLightIntersection* pHead) const
{
	// NB: tfm change can be important for terrain, as light may be still intersecting with the terrain in a whole but now affecting other patches.
	// Terrain could also remember light's bounds version? Make sure we call UpdateLightList for terrain at any tfm change.

	auto pTerrain = static_cast<Render::CTerrain*>(&Renderable);
	while (pHead)
	{
		// TODO: use visible patches calculated in UpdateRenderable and AABB tree to calculate lights per patch

		pHead = pHead->pNextLight;
	}
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
