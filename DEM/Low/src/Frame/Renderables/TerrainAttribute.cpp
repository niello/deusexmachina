#include "TerrainAttribute.h"
#include <Frame/View.h>
#include <Frame/GraphicsResourceManager.h>
#include <Frame/CameraAttribute.h>
#include <Frame/LightAttribute.h>
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
#include <Scene/SceneNode.h>
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
	ClonedAttr->_CDLODData = _CDLODData;
	ClonedAttr->_MaterialUID = _MaterialUID;
	ClonedAttr->_CDLODDataUID = _CDLODDataUID;
	ClonedAttr->_HeightMapUID = _HeightMapUID;
	ClonedAttr->_InvSplatSizeX = _InvSplatSizeX;
	ClonedAttr->_InvSplatSizeZ = _InvSplatSizeZ;
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

void CTerrainAttribute::UpdateRenderable(CView& View, Render::IRenderable& Renderable, bool ViewProjChanged) const
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
	bool DataChanged = false;
	if (!_CDLODData)
	{
		DataChanged = true;
		pTerrain->CDLODData = nullptr;
		pTerrain->PatchMesh = nullptr;
		pTerrain->QuarterPatchMesh = nullptr;
		pTerrain->GeometryKey = 0;
	}
	else if (!pTerrain->CDLODData || pTerrain->CDLODData != _CDLODData)
	{
		DataChanged = true;
		pTerrain->CDLODData = _CDLODData;

		const auto PatchSize = _CDLODData->GetPatchSize();
		//if (!Math::IsPow2(PatchSize) || PatchSize < 4) FAIL;

		//!!!TODO: store both patches in one mesh, only use different primitive groups (can vary only indices! whole indices and quarter indices)
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

		n_assert_dbg(pTerrain->PatchMesh->GetVertexBuffer()->GetVertexLayout() == pTerrain->QuarterPatchMesh->GetVertexBuffer()->GetVertexLayout());

		pTerrain->GeometryKey = pTerrain->PatchMesh->GetSortingKey();
	}

	// Precalculate LOD morphing constants
	const float VisibilityRange = View.GetCamera()->GetFarPlane();
	const bool MorphChanged = (DataChanged || pTerrain->VisibilityRange != VisibilityRange);
	if (MorphChanged)
	{
		pTerrain->VisibilityRange = VisibilityRange;

		//???!!!to terrain settings or to renderer settings or to view params!?
		//!!!clamp to range 0.5f .. 0.95f!
		constexpr float MorphStartRatio = 0.7f;

		const U32 LODCount = pTerrain->GetCDLODData()->GetLODCount();
		pTerrain->LODParams.resize(LODCount);
		pTerrain->LODParams.shrink_to_fit();
		if (LODCount)
		{
			float MorphStart = 0.f;
			for (U32 LOD = 0; LOD < LODCount; ++LOD)
			{
				float LODRange = VisibilityRange / static_cast<float>(1 << (LODCount - 1 - LOD));

				// Hack, see original CDLOD code. LOD 0 range is 0.9 of what is expected.
				if (!LOD) LODRange *= 0.9f;

				MorphStart = n_lerp(MorphStart, LODRange, MorphStartRatio);
				const float MorphEnd = n_lerp(LODRange, MorphStart, 0.01f);

				auto& LODParams = pTerrain->LODParams[LOD];
				LODParams.Range = LODRange;
				LODParams.Morph2 = 1.0f / (MorphEnd - MorphStart);
				LODParams.Morph1 = MorphEnd * LODParams.Morph2;
			}
		}
	}

	// Update a list of visible patches
	if (MorphChanged || ViewProjChanged || pTerrain->PatchesTransformVersion != _pNode->GetTransformVersion())
	{
		pTerrain->PatchesTransformVersion = _pNode->GetTransformVersion();
		pTerrain->UpdatePatches(View.GetCamera()->GetPosition(), View.GetCamera()->GetViewProjMatrix());

		//???set all lights dirty? or update now?
		//could merge / divide existing nodes and/or use finest LOD light grid

		//???store main info about patches in one buffer, lights in another?!
		// - will save buffer refreshes when only light data changes
		// - depth will bind only main data, no lights
		// - more data will fit into constant buffers w/out structured buffers
		// - maybe better layout
		// - different shaders use it! VS / PS.
	}
}
//---------------------------------------------------------------------

void CTerrainAttribute::UpdateLightList(CView& View, Render::IRenderable& Renderable, const CObjectLightIntersection* pHead) const
{
	//???store finest LOD light grid in CTerrainAttribute? scene data, not view! Use in all views then!
	//!!!need a method without IRenderable for that?! not to repeat this scene based processing per view!

	// NB: tfm change can be important for terrain, as light may be still intersecting with the terrain in a whole but now affecting other patches.
	// Terrain could also remember light's bounds version? Make sure we call UpdateLightList for terrain at any tfm change.

	//???can update only changed intersections and not all lights each time one of them changed?! use renderable tfm versions from intersections?

	//???sorted sync like in model attr? then mark removed lights for updating indices in patches, and update added lights and lights with changed bounds

	auto pTerrain = static_cast<Render::CTerrain*>(&Renderable);
	while (pHead)
	{
		// TODO: use visible patches calculated in UpdateRenderable and AABB tree to calculate lights per patch

		pHead = pHead->pNextLight;
	}
}
//---------------------------------------------------------------------

//!!!need to detect if light moved OR renderable itself moved! In both cases must update lights in a quadtree!
void CTerrainAttribute::OnLightIntersectionsUpdated()
{
	// Terrain is too big to process all intersecting lights on the whole surface. Here is additional processing
	// on the scene level to detect affected region for each light. This data is then used by all views.
	const auto& Record = GetSceneHandle()->second;
	if (_LightCacheIntersectionsVersion == Record.ObjectLightIntersectionsVersion) return;
	_LightCacheIntersectionsVersion = Record.ObjectLightIntersectionsVersion;

	const bool TerrainMoved = (_LightCacheBoundsVersion != Record.BoundsVersion);
	_LightCacheBoundsVersion = Record.BoundsVersion;

	// Sync sorted light list from intersections to renderable. Uses manual specification of DEM::Algo::SortedUnion.
	auto It = _Lights.begin();
	const CObjectLightIntersection* pCurrIsect = Record.pObjectLightIntersections;
	while ((It != _Lights.cend()) || pCurrIsect)
	{
		if (!pCurrIsect || ((It != _Lights.cend()) && It->first < pCurrIsect->pLightAttr->GetSceneHandle()->first))
		{
			It = _Lights.erase(It); //!!!TODO PERF: use shared node pool!

			// actual changes exist only if the light had non-empty list of patches affected!
			//???return bool from this function to indicate actual changes? but how to propagate to all views?!
		}
		else if ((It == _Lights.cend()) || (pCurrIsect && pCurrIsect->pLightAttr->GetSceneHandle()->first < It->first))
		{
			const auto UID = pCurrIsect->pLightAttr->GetSceneHandle()->first;

			//_Lights.emplace_hint(It, UID, View.GetLight(UID)); //!!!TODO PERF: use shared node pool!

			// update light affection zone. unchanged if the light doesn't touch any patch actually (e.g. it is above the ground)
			//???return bool from this function to indicate actual changes? but how to propagate to all views?!

			pCurrIsect = pCurrIsect->pNextLight;
		}
		else // equal
		{
			if (TerrainMoved || It->second.BoundsVersion != pCurrIsect->LightBoundsVersion)
			{
				// update light affection zone, note that it may remain the same! can avoid resetting values in views then?!
				//???return bool from this function to indicate actual changes? but how to propagate to all views?!
			}

			++It;
			pCurrIsect = pCurrIsect->pNextLight;
		}
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

void CTerrainAttribute::OnActivityChanged(bool Active)
{
	// Invalidate the light cache as soon as the scene stops updating the terrain
	if (!Active) _Lights.clear();
	CRenderableAttribute::OnActivityChanged(Active);
}
//---------------------------------------------------------------------

}
