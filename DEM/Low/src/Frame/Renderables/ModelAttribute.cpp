#include "ModelAttribute.h"
#include <Frame/GraphicsResourceManager.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <Render/Model.h>
#include <Render/Mesh.h>
#include <Render/MeshData.h>
#include <Render/Material.h>
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

void CModelAttribute::UpdateRenderable(CGraphicsResourceManager& ResMgr, Render::IRenderable& Renderable) const
{
	//!!!TODO: pass LOD from outside!
	//???pass only LOD metric and calc LODs for geom & mtl from it and from attr settings?!
	UPTR LOD = 0;

	auto pModel = static_cast<Render::CModel*>(&Renderable);

	//!!!TODO: setup renderer!

	// Initialize geometry
	if (!_MeshUID)
	{
		if (pModel->Mesh)
		{
			pModel->Mesh = nullptr;
			pModel->pGroup = nullptr;
		}
	}
	else if (!pModel->Mesh || pModel->Mesh->GetUID() != _MeshUID)
	{
		pModel->Mesh = ResMgr.GetMesh(_MeshUID);
		pModel->pGroup = _MeshData->GetGroup(_MeshGroupIndex, LOD);
	}

	// Initialize material
	if (!_MaterialUID)
	{
		if (pModel->Material)
		{
			pModel->Material = nullptr;
			// erase effect & tech cache
		}
	}
	else if (!pModel->Material || pModel->Material->GetUID() != _MaterialUID)
	{
		// TODO: use LOD to choose a material from set!
		pModel->Material = ResMgr.GetMaterial(_MaterialUID);
		// init effect & tech cache, using input set from renderer! do everything inside renderer, passing only us and the material? return tech record index.
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
