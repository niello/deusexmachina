#include "ModelAttribute.h"
#include <Frame/View.h>
#include <Frame/GraphicsResourceManager.h>
#include <Frame/SkinAttribute.h>
#include <Frame/SkinProcessorAttribute.h>
#include <Frame/LightAttribute.h>
#include <Scene/SceneNode.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <Render/Model.h>
#include <Render/Mesh.h>
#include <Render/MeshData.h>
#include <Render/Material.h>
#include <Render/Effect.h>
#include <Render/SkinInfo.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Frame
{
FACTORY_CLASS_IMPL(Frame::CModelAttribute, 'MDLA', Frame::CRenderableAttribute);

CModelAttribute::CModelAttribute(CStrID MeshUID, CStrID MaterialUID, U32 MeshGroupIndex)
	: _MeshUID(MeshUID)
	, _MaterialUID(MaterialUID)
	, _MeshGroupIndex(MeshGroupIndex)
{
}
//---------------------------------------------------------------------

bool CModelAttribute::LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count)
{
	for (UPTR j = 0; j < Count; ++j)
	{
		const uint32_t Code = DataReader.Read<uint32_t>();
		switch (Code)
		{
			case 'MTRL':
			{
				_MaterialUID = DataReader.Read<CStrID>();
				break;
			}
			case 'MESH':
			{
				_MeshUID = DataReader.Read<CStrID>();
				break;
			}
			case 'MSGR':
			{
				if (!DataReader.Read(_MeshGroupIndex)) FAIL;
				break;
			}
			default: FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CModelAttribute::Clone()
{
	PModelAttribute ClonedAttr = n_new(CModelAttribute());
	ClonedAttr->_MeshUID = _MeshUID;
	ClonedAttr->_MaterialUID = _MaterialUID;
	ClonedAttr->_MeshGroupIndex = _MeshGroupIndex;
	ClonedAttr->_MeshData = _MeshData;
	return ClonedAttr;
}
//---------------------------------------------------------------------

bool CModelAttribute::ValidateResources(Resources::CResourceManager& ResMgr)
{
	// Store mesh data pointer for GPU-independent local AABB access
	if (!_MeshData)
	{
		Resources::PResource RMeshData = ResMgr.RegisterResource<Render::CMeshData>(_MeshUID.CStr());
		_MeshData = RMeshData ? RMeshData->ValidateObject<Render::CMeshData>() : nullptr;
	}
	OK;
}
//---------------------------------------------------------------------

Render::PRenderable CModelAttribute::CreateRenderable() const
{
	return std::make_unique<Render::CModel>();
}
//---------------------------------------------------------------------

void CModelAttribute::UpdateRenderable(CView& View, Render::IRenderable& Renderable, bool /*ViewProjChanged*/) const
{
	//!!!TODO: calc LOD from Renderable.SqDistanceToCamera and from screen radius (to be added to Renderable)!
	//!!!NB: object can be culled by LOD (i.e. by distance or screen size). Then need to set Renderable.IsVisible here to false and return.
	UPTR LOD = 0;	

	auto pModel = static_cast<Render::CModel*>(&Renderable);

	// Initialize geometry
	if (!_MeshUID)
	{
		if (pModel->Mesh)
		{
			pModel->Mesh = nullptr;
			pModel->pGroup = nullptr;
			pModel->GeometryKey = 0;
		}
	}
	else if (!pModel->Mesh || pModel->Mesh->GetUID() != _MeshUID) //!!! || LOD != _LOD
	{
		pModel->Mesh = View.GetGraphicsManager()->GetMesh(_MeshUID);
		pModel->pGroup = _MeshData->GetGroup(_MeshGroupIndex, LOD);
		pModel->GeometryKey = pModel->Mesh->GetSortingKey() + pModel->pGroup->IndexInMesh;
	}

	static const CStrID InputSet_Model("Model");
	static const CStrID InputSet_ModelSkinned("ModelSkinned");
	const auto PrevInputSet = pModel->pSkinPalette ? InputSet_ModelSkinned : InputSet_Model;

	// Find a skin palette
	// FIXME: when to update? Don't want to check this each frame! Could check in CreateRenderable(), but then need to detect
	// CSkinAttribute destruction and also can't react on adding or changing it on the fly!
	//???embed optional skin into a CModelAttribute? Or even make a subclass CSkinnedModelAttribute : public CModelAttribute?
	if (auto pSkinAttr = _pNode->FindFirstAttribute<Frame::CSkinAttribute>())
	{
		if (const auto& Palette = pSkinAttr->GetSkinPalette())
		{
			pModel->pSkinPalette = Palette->GetSkinPalette();
			pModel->BoneCount = Palette->GetSkinInfo()->GetBoneCount();
		}
	}
	else pModel->pSkinPalette = nullptr;

	const auto InputSet = pModel->pSkinPalette ? InputSet_ModelSkinned : InputSet_Model;

	// Initialize material
	if (!_MaterialUID)
	{
		if (pModel->Material)
		{ 
			pModel->Material = nullptr;
			pModel->ShaderTechIndex = INVALID_INDEX_T<U32>;
			pModel->RenderQueueMask = 0;
			pModel->MaterialKey = 0;
			pModel->ShaderTechKey = 0;
		}
	}
	else if (!pModel->Material || pModel->Material->GetUID() != _MaterialUID || PrevInputSet != InputSet) //!!! || LOD != RememberedLOD
	{
		// TODO: use LOD to choose a material from set!
		pModel->Material = View.GetGraphicsManager()->GetMaterial(_MaterialUID);
		if (pModel->Material && pModel->Material->GetEffect())
		{
			pModel->ShaderTechIndex = View.RegisterEffect(*pModel->Material->GetEffect(), InputSet);
			pModel->RenderQueueMask = (1 << pModel->Material->GetEffect()->GetType());
			pModel->MaterialKey = pModel->Material->GetSortingKey();
			pModel->ShaderTechKey = View.GetShaderTechCache()[pModel->ShaderTechIndex]->GetSortingKey(); //???FIXME: now we use non-overridden tech key for all phases
		}
	}
}
//---------------------------------------------------------------------

void CModelAttribute::UpdateLightList(CView& View, Render::IRenderable& Renderable, const CObjectLightIntersection* pHead) const
{
	auto pModel = static_cast<Render::CModel*>(&Renderable);

	// Sync sorted light list from intersections to renderable. Uses manual specialization of DEM::Algo::SortedUnion.
	auto It = pModel->Lights.begin();
	auto pCurrIsect = pHead;
	while ((It != pModel->Lights.cend()) || pCurrIsect)
	{
		if (!pCurrIsect || ((It != pModel->Lights.cend()) && It->first < pCurrIsect->pLightAttr->GetSceneHandle()->first))
		{
			It = pModel->Lights.erase(It); //!!!TODO PERF: use shared node pool, one for all models!
		}
		else if ((It == pModel->Lights.cend()) || (pCurrIsect && pCurrIsect->pLightAttr->GetSceneHandle()->first < It->first))
		{
			const auto UID = pCurrIsect->pLightAttr->GetSceneHandle()->first;
			pModel->Lights.emplace_hint(It, UID, View.GetLight(UID)); //!!!TODO PERF: use shared node pool, one for all models!
			pCurrIsect = pCurrIsect->pNextLight;
		}
		else // equal
		{
			++It;
			pCurrIsect = pCurrIsect->pNextLight;
		}
	}
}
//---------------------------------------------------------------------

bool CModelAttribute::GetLocalAABB(CAABB& OutBox, UPTR LOD) const
{
	if (!_MeshData) FAIL;

	const Render::CPrimitiveGroup* pGroup = _MeshData->GetGroup(_MeshGroupIndex, LOD);
	if (!pGroup) FAIL;

	OutBox = pGroup->AABB;
	OK;
}
//---------------------------------------------------------------------

}
