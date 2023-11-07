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
		pTerrain->Material = ResMgr.GetMaterial(_MaterialUID);
		if (pTerrain->Material && pTerrain->Material->GetEffect())
		{
			static const CStrID InputSet_CDLOD("CDLOD");

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
		n_assert_dbg(Math::IsPow2(PatchSize));

		//!!!TODO: store both patches in one mesh, only use different primitive groups (can vary only indices! whole indices and quarter indices)
		//???maybe render 4 quarterpatches instead of 1 whole? then will be 1 DIP per terrain cluster, with a possibility to merge!
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

		const U32 LODCount = _CDLODData ? _CDLODData->GetLODCount() : 0;
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
		// FIXME: mast pass the main camera position, the one used for final frame presentation, not a view camera pos!
		pTerrain->PatchesTransformVersion = _pNode->GetTransformVersion();
		pTerrain->UpdatePatches(View.GetCamera()->GetPosition(), View.GetViewFrustum());

		//???how to make sure that UpdateLightList will be called? World state might be unchanged but
		//patches could be changed due to camera movement. Call sync manually here?
	}
}
//---------------------------------------------------------------------

void CTerrainAttribute::UpdateLightList(CView& View, Render::IRenderable& Renderable, const CObjectLightIntersection* /*pHead*/) const
{
	auto pTerrain = static_cast<Render::CTerrain*>(&Renderable);

	// for each visible patch and quarterpatch
	//  if its LOD > pTerrain->MaxDynamicallyLitLOD, clear light list of the patch by setting invalid GPU index and continue
	//  find node in _Nodes by patchs Morton code (sorted sync aka left join or unordered map access or something better?)
	//  if node not found but patch has lights, clear its lights and set version to zero
	//  if node version != patch light list version, update lights in a patch from node's light UID list and update version

	//???store lights in a CPatchInstance or write directly to CB?! Always sorted before filling lights, so can write by index! Commit CB after sync.
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
			// Erase this light from all previously affected nodes
			for (TMorton NodeCode : It->second.AffectedNodes)
				StopAffectingNode(NodeCode, It->second.pLightAttr);

			It = _Lights.erase(It); //!!!TODO PERF: use shared node pool!
		}
		else if ((It == _Lights.cend()) || (pCurrIsect && pCurrIsect->pLightAttr->GetSceneHandle()->first < It->first))
		{
			const auto UID = pCurrIsect->pLightAttr->GetSceneHandle()->first;
			auto& LightInfo = _Lights.emplace_hint(It, UID, CLightInfo{})->second; //!!!TODO PERF: use shared node pool!
			LightInfo.pLightAttr = pCurrIsect->pLightAttr;
			UpdateLightInQuadTree(LightInfo);
			LightInfo.BoundsVersion = pCurrIsect->LightBoundsVersion;
			pCurrIsect = pCurrIsect->pNextLight;
		}
		else // equal
		{
			if (TerrainMoved || It->second.BoundsVersion != pCurrIsect->LightBoundsVersion)
			{
				UpdateLightInQuadTree(It->second);
				It->second.BoundsVersion = pCurrIsect->LightBoundsVersion;
			}

			++It;
			pCurrIsect = pCurrIsect->pNextLight;
		}
	}
}
//---------------------------------------------------------------------

void CTerrainAttribute::StartAffectingNode(TMorton NodeCode, CLightAttribute* pLightAttr)
{
	auto ItNode = _Nodes.find(NodeCode);
	if (ItNode == _Nodes.cend())
		ItNode = _Nodes.emplace().first; //!!!TODO PERF: use shared node pool!

	ItNode->second.Lights.emplace(pLightAttr); //!!!TODO PERF: use shared node pool!
	++ItNode->second.Version;
}
//---------------------------------------------------------------------

void CTerrainAttribute::StopAffectingNode(TMorton NodeCode, CLightAttribute* pLightAttr)
{
	auto ItNode = _Nodes.find(NodeCode);
	if (ItNode == _Nodes.cend()) return;

	if (ItNode->second.Lights.erase(pLightAttr)) //!!!TODO PERF: use shared node pool!
	{
		if (ItNode->second.Lights.empty())
			_Nodes.erase(ItNode); //!!!TODO PERF: use shared node pool!
		else
			++ItNode->second.Version;
	}
}
//---------------------------------------------------------------------

void CTerrainAttribute::UpdateLightInQuadTree(CLightInfo& LightInfo)
{
	// TODO PERF: could have a pool of these vectors and swap with pool record! Or even have one tmp buffer if only single thread accesses each attr.
	std::vector<TMorton> PrevAffectedNodes;
	std::swap(PrevAffectedNodes, LightInfo.AffectedNodes);

	CNodeProcessingContext Ctx;
	Ctx.Scale = Math::ToSIMD(_pNode->GetWorldMatrix().ExtractScale());
	Ctx.Offset = Math::ToSIMD(_pNode->GetWorldMatrix().Translation());
	Ctx.pLightInfo = &LightInfo;

	UpdateLightInQuadTreeNode(Ctx, 0, 0, _CDLODData->GetLODCount() - 1);

	std::sort(LightInfo.AffectedNodes.begin(), LightInfo.AffectedNodes.end());

	DEM::Algo::SortedUnion(PrevAffectedNodes, LightInfo.AffectedNodes,
		[this, &PrevAffectedNodes, &LightInfo](auto ItPrev, auto ItNew)
	{
		if (ItPrev == PrevAffectedNodes.cend())
			StartAffectingNode(*ItNew, LightInfo.pLightAttr);
		else if (ItNew == LightInfo.AffectedNodes.cend())
			StopAffectingNode(*ItPrev, LightInfo.pLightAttr);
	});

	// TODO: return PrevAffectedNodes to the pool / keep as a tmp buffer!
}
//---------------------------------------------------------------------

bool CTerrainAttribute::UpdateLightInQuadTreeNode(const CNodeProcessingContext& Ctx, TCellDim x, TCellDim z, U32 LOD)
{
	// Calculate node world space AABB
	acl::Vector4_32 BoxCenter, BoxExtent;
	if (!_CDLODData->GetNodeAABB(x, z, LOD, BoxCenter, BoxExtent)) return false;
	BoxExtent = acl::vector_mul(BoxExtent, Ctx.Scale);
	BoxCenter = acl::vector_add(BoxCenter, Ctx.Offset);

	const auto ClipStatus = Ctx.pLightInfo->pLightAttr->TestBoxClipping(BoxCenter, BoxExtent);

	// If the whole node is outside the light, skip its subtree as not affected
	if (ClipStatus == Math::ClipOutside) return false;

	// If the whole node is inside the light, add the whole subtree and skip its recursive traversal
	if (ClipStatus == Math::ClipInside)
	{
		// Cut subtree top if it is above the max dynamically lit LOD
		const auto StartSubtreeLevel = LOD - std::min<U32>(LOD, _MaxLODForDynamicLights);

		// Reserve elements for dynamically lit levels. May be a bit excessive because CDLOD levels may contain less cols & rows than pow2.
		Ctx.pLightInfo->AffectedNodes.reserve(Math::GetQuadtreeNodeCount(LOD + 1) - Math::GetQuadtreeNodeCount(StartSubtreeLevel));

		// Add the current node all its existing children.
		// TODO: investigate ways to optimize this using properties of Morton codes. Query all Morton codes in a rect.
		const auto BaseDepth = _CDLODData->GetLODCount() - 1 - LOD;
		for (U32 SubtreeLevel = StartSubtreeLevel; SubtreeLevel < LOD; ++SubtreeLevel)
		{
			const auto Depth = BaseDepth + SubtreeLevel;
			const auto DepthBit = (1 << Depth << Depth);

			const auto LODSize = _CDLODData->GetLODSize(LOD - SubtreeLevel);
			const TCellDim QuadsDim = (1 << SubtreeLevel);
			const TCellDim FromX = x << SubtreeLevel;
			const TCellDim FromZ = z << SubtreeLevel;
			const TCellDim ToX = std::min<TCellDim>(FromX + QuadsDim, LODSize.first);
			const TCellDim ToZ = std::min<TCellDim>(FromZ + QuadsDim, LODSize.second);

			for (auto CurrZ = FromZ; CurrZ <= ToZ; ++CurrZ)
				for (auto CurrX = FromX; CurrX <= ToX; ++CurrX)
					Ctx.pLightInfo->AffectedNodes.push_back(DepthBit | Math::MortonCode2(CurrX, CurrZ));
		}

		return true;
	}

	// For non-leaf nodes descend to children
	if (LOD > 0)
	{
		const auto [HasRightChild, HasBottomChild] = _CDLODData->GetChildExistence(x, z, LOD);

		const TCellDim NextX = x << 1;
		const TCellDim NextZ = z << 1;
		const U32 NextLOD = LOD - 1;

		const bool IsectLT = UpdateLightInQuadTreeNode(Ctx, NextX, NextZ, NextLOD);
		const bool IsectRT = (HasRightChild && UpdateLightInQuadTreeNode(Ctx, NextX + 1, NextZ, NextLOD));
		const bool IsectLB = (HasBottomChild && UpdateLightInQuadTreeNode(Ctx, NextX, NextZ + 1, NextLOD));
		const bool IsectRB = (HasRightChild && HasBottomChild && UpdateLightInQuadTreeNode(Ctx, NextX + 1, NextZ + 1, NextLOD));

		// If no children are affected by the light, parent node is neither considered affected
		if (!IsectLT && !IsectRT && !IsectLB && !IsectRB) return false;

		// Don't track lights for too coarse LODs. It is good for two reasons:
		// 1. The farther the terrain patch is, the less importaint and less noticeable is its proper lighting with typically relatively small dynamic lights.
		// 2. Far terrain patches are very big and would have lots of affecting lights, loading the light tracker with unnecessary work.
		if (LOD > _MaxLODForDynamicLights) return true;
	}

	// If we are here, we are affected by the light and want to track it
	const auto Depth = _CDLODData->GetLODCount() - 1 - LOD;
	Ctx.pLightInfo->AffectedNodes.push_back((1 << Depth << Depth) | Math::MortonCode2(x, z));
	return true;
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
	if (!Active)
	{
		_Lights.clear();
		_Nodes.clear();
		_LightCacheBoundsVersion = 0;
		_LightCacheIntersectionsVersion = 0;
	}

	CRenderableAttribute::OnActivityChanged(Active);
}
//---------------------------------------------------------------------

}
