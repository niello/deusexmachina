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

		//???set all lights dirty in a CTerrain renderable view? or update now?
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
	//!!!can limit lighting to certain terrain patch LODs! don't process dynamic lights for huge far patches. Saves lots of work
	//because big patches tend to intersect with very many lights.

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
			//Changed |= !It->second.Nodes.empty();
			It = _Lights.erase(It); //!!!TODO PERF: use shared node pool!
		}
		else if ((It == _Lights.cend()) || (pCurrIsect && pCurrIsect->pLightAttr->GetSceneHandle()->first < It->first))
		{
			const auto UID = pCurrIsect->pLightAttr->GetSceneHandle()->first;
			auto& LightInfo = _Lights.emplace_hint(It, UID, CLightInfo{})->second; //!!!TODO PERF: use shared node pool!
			UpdateLightInQuadTree(pCurrIsect->pLightAttr, true);
			pCurrIsect = pCurrIsect->pNextLight;
		}
		else // equal
		{
			if (TerrainMoved || It->second.BoundsVersion != pCurrIsect->LightBoundsVersion)
			{
				// extract current list of light's nodes
				UpdateLightInQuadTree(pCurrIsect->pLightAttr, false);
				// clear light from nodes that are in old list but aren't in new
			}

			++It;
			pCurrIsect = pCurrIsect->pNextLight;
		}
	}

	// to test light vs node, can first test light bounds (integer math?) and then search if any of node's LOD0 cells are in the list
	// could do 2 searches to get lower (first its LOD0) & upper bound (last its LOD0), and if the range is not empty, light affects it
	// for first bounds test can store integer range in each light info record

	//???track nodes in a light or fill sparse quadtree with lights affecting the quad?

	//???return bool from this function to indicate actual changes? but how to propagate to all views?!
	// could track coverage version in a terrain attr and in terrain renderable views!
	//???store version in each light? can update only certain lights! could make everything much cheaper!
	//instead of Changed flag, need to precalculate NextVersion = CurrVersion + 1 and assign it to changed lights.
	//it is not an own version for each light, it is the same version marking last change moment of the light
	//knowing changed lights we know which LOD0 nodes should be updated, others can be kept

	//when light stops affecting some node, need to detect that and clean its index from node's light list.
}
//---------------------------------------------------------------------

bool CTerrainAttribute::UpdateLightInQuadTree(const CLightAttribute* pLightAttr, bool NewLight)
{
	constexpr U32 RootMortonCode = 1; // Depth bit at 0th position

	CAABB AABB = _CDLODData->GetAABB();
	AABB.Transform(_pNode->GetWorldMatrix());

	/*
	Ctx.ScaleBaseX = (AABB.Max.x - AABB.Min.x) / static_cast<float>(CDLODData->GetHeightMapWidth() - 1);
	Ctx.ScaleBaseZ = (AABB.Max.z - AABB.Min.z) / static_cast<float>(CDLODData->GetHeightMapHeight() - 1);
	*/

	return UpdateLightInQuadTreeNode(pLightAttr, NewLight, RootMortonCode);
}
//---------------------------------------------------------------------

// Returns if light list in any of quadtree nodes changed
bool CTerrainAttribute::UpdateLightInQuadTreeNode(const CLightAttribute* pLightAttr, bool NewLight, U32 MortonCode)
{
	/*
	I16 MinY, MaxY;
	CDLODData->GetMinMaxHeight(X, Z, LOD, MinY, MaxY);

	// Node has no data, skip it completely
	if (MaxY < MinY) return ENodeStatus::Invisible;

	const U32 NodeSize = CDLODData->GetPatchSize() << LOD;
	const float ScaleX = NodeSize * Ctx.ScaleBaseX;
	const float ScaleZ = NodeSize * Ctx.ScaleBaseZ;
	const float NodeMinX = Ctx.AABB.Min.x + X * ScaleX;
	const float NodeMinZ = Ctx.AABB.Min.z + Z * ScaleZ;

	CAABB NodeAABB;
	NodeAABB.Min.x = NodeMinX;
	NodeAABB.Min.y = MinY * CDLODData->GetVerticalScale();
	NodeAABB.Min.z = NodeMinZ;
	NodeAABB.Max.x = NodeMinX + ScaleX;
	NodeAABB.Max.y = MaxY * CDLODData->GetVerticalScale();
	NodeAABB.Max.z = NodeMinZ + ScaleZ;
	*/

	const auto& LightSceneRecord = pLightAttr->GetSceneHandle()->second;

	bool Intersect;
	const U32 DeepestLOD = _CDLODData->GetLODCount() - 1;
	const U32 LeafMortonStart = 1 << DeepestLOD << DeepestLOD;
	if (MortonCode < LeafMortonStart)
	{
		Intersect = false;
		U32 ChildCode = MortonCode << 2;
		const U32 ChildEndCode = ChildCode + 4;
		for (; ChildCode < ChildEndCode; ++ChildCode)
		{
			//!!!TODO: if light bounds don't intersect with this child, skip traversing it!
			//???use Clipped/Inside? If node is inside the light, all children are intersected by this light!
			//!!!this is not exactly true because children have different Y! the only guarantee is that if there is no isect, children don't isect too!

			Intersect |= UpdateLightInQuadTreeNode(pLightAttr, NewLight, ChildCode);
		}
	}
	else
	{
		Intersect = false;
		//BOX = get quadtree node bounds
		//Intersect = pLightAttr->IntersectsWith(BOX);
		//Intersect = Math::ClipSphere(LightSceneRecord.Sphere, BOX);
	}

	//!!!can skip saving light lists in nodes after some level, regardless of what is the finest LOD with dynamic lights in views.
	//???could store param in CLightAttribute?! MaxDynamicallyLitLOD. Avoid collecting huge lists in coarsest nodes!

	if (Intersect)
	{
		auto ItNode = _Nodes.find(MortonCode);
		if (ItNode == _Nodes.cend())
		{
			// add to _Nodes, get node handle from pool
		}
		else
		{
			// check if this light already added to this cell
			//???or will know it when trying to emplace? will implace in this case create and destroy unnecessary map node?
			//as a fallback can do find and then emplace_hint, but if not found, cend() is not a good hint for sorted insertion! use lower_bound?
		}
		// add light to ItNode->second.Lights, get node handle from pool
		// if added (and didn't already exist), up node version
	}
	else if (!NewLight)
	{
		auto ItNode = _Nodes.find(MortonCode);
		if (ItNode != _Nodes.cend())
		{
			auto& NodeLights = ItNode->second.Lights;
			// try remove light from NodeLights
			// if removed
			{
				// put extracted light map node into the pool

				if (NodeLights.empty()) //!!!instead of erasing, put map node handle into the pool!
				{
					_Nodes.erase(ItNode);
				}
				else
				{
					// up node version
				}
			}
		}
	}

	//???write LightBoundsVersion or ObjectLightIntersectionsVersion or special version to nodes? version must be incremented when terrain moved too!

	return Intersect;
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
