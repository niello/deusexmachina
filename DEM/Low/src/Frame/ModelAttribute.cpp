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
{
	Renderable.reset(n_new(Render::CModel()));
	auto pModel = static_cast<Render::CModel*>(Renderable.get());
	pModel->MeshGroupIndex = MeshGroupIndex;
}
//---------------------------------------------------------------------

bool CModelAttribute::LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count)
{
	if (!Renderable) Renderable.reset(n_new(Render::CModel()));
	auto pModel = static_cast<Render::CModel*>(Renderable.get());

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
				if (!DataReader.Read(pModel->MeshGroupIndex)) FAIL;
				break;
			}
			case 'JPLT':
			{
				U16 Count;
				if (!DataReader.Read(Count)) FAIL;
				pModel->BoneIndices.SetSize(Count);
				if (DataReader.GetStream().Read(pModel->BoneIndices.GetPtr(), Count * sizeof(int)) != Count * sizeof(int)) FAIL;
				break;
			}
			default: FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool CModelAttribute::ValidateResources(Resources::CResourceManager& ResMgr)
{
	if (!Renderable) Renderable.reset(n_new(Render::CModel()));
	auto pModel = static_cast<Render::CModel*>(Renderable.get());

	// Store mesh data pointer for GPU-independent local AABB access
	Resources::PResource RMeshData = ResMgr.RegisterResource<Render::CMeshData>(_MeshUID);
	pModel->MeshData = RMeshData ? RMeshData->ValidateObject<Render::CMeshData>() : nullptr;

	OK;
}
//---------------------------------------------------------------------

bool CModelAttribute::ValidateGPUResources(CGraphicsResourceManager& ResMgr)
{
	if (!Renderable) Renderable.reset(n_new(Render::CModel()));
	auto pModel = static_cast<Render::CModel*>(Renderable.get());

	pModel->Mesh = _MeshUID ? ResMgr.GetMesh(_MeshUID) : nullptr;
	pModel->Material = _MaterialUID ? ResMgr.GetMaterial(_MaterialUID) : nullptr;

	OK;
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CModelAttribute::Clone()
{
	PModelAttribute ClonedAttr = n_new(CModelAttribute());
	if (Renderable) ClonedAttr->Renderable = std::move(Renderable->Clone());
	ClonedAttr->_MeshUID = _MeshUID;
	ClonedAttr->_MaterialUID = _MaterialUID;
	return ClonedAttr;
}
//---------------------------------------------------------------------

}