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
			case 'JPLT':
			{
				U16 Count;
				if (!DataReader.Read(Count)) FAIL;
				_BoneIndices.SetSize(Count);
				if (DataReader.GetStream().Read(_BoneIndices.GetPtr(), Count * sizeof(int)) != Count * sizeof(int)) FAIL;
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
	ClonedAttr->_BoneIndices = _BoneIndices;
	ClonedAttr->_MeshData = _MeshData;
	return ClonedAttr;
}
//---------------------------------------------------------------------

bool CModelAttribute::ValidateResources(Resources::CResourceManager& ResMgr)
{
	// Store mesh data pointer for GPU-independent local AABB access
	if (!_MeshData)
	{
		Resources::PResource RMeshData = ResMgr.RegisterResource<Render::CMeshData>(_MeshUID);
		_MeshData = RMeshData ? RMeshData->ValidateObject<Render::CMeshData>() : nullptr;
	}
	OK;
}
//---------------------------------------------------------------------

Render::PRenderable CModelAttribute::CreateRenderable(CGraphicsResourceManager& ResMgr) const
{
	auto pModel = n_new(Render::CModel());

	// TODO: don't copy, store once!
	pModel->BoneIndices = _BoneIndices;
	pModel->MeshGroupIndex = _MeshGroupIndex;

	pModel->Mesh = _MeshUID ? ResMgr.GetMesh(_MeshUID) : nullptr;
	pModel->Material = _MaterialUID ? ResMgr.GetMaterial(_MaterialUID) : nullptr;

	return Render::PRenderable(pModel);
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