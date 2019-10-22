#include "ModelAttribute.h"
#include <Frame/GraphicsResourceManager.h>
#include <Render/Model.h>
#include <Render/Mesh.h>
#include <Render/Material.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Frame
{
__ImplementClass(Frame::CModelAttribute, 'MDLA', Frame::CNodeAttrRenderable);

bool CModelAttribute::LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count)
{
	if (!Renderable) Renderable.reset(n_new(Render::CModel));
	auto pModel = static_cast<Render::CModel*>(Renderable.get());

	for (UPTR j = 0; j < Count; ++j)
	{
		const uint32_t Code = DataReader.Read<uint32_t>();
		switch (Code)
		{
			case 'MTRL':
			{
				MaterialUID = DataReader.Read<CStrID>();
				break;
			}
			case 'MESH':
			{
				MeshUID = DataReader.Read<CStrID>();
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

bool CModelAttribute::ValidateResources(CGraphicsResourceManager& ResMgr)
{
	if (!Renderable) FAIL;

	auto pModel = static_cast<Render::CModel*>(Renderable.get());
	pModel->Mesh = MeshUID ? ResMgr.GetMesh(MeshUID) : nullptr;
	pModel->Material = MaterialUID ? ResMgr.GetMaterial(MaterialUID) : nullptr;
	OK;
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CModelAttribute::Clone()
{
	PModelAttribute ClonedAttr = n_new(CModelAttribute);
	if (Renderable) ClonedAttr->Renderable = std::move(Renderable->Clone());
	ClonedAttr->MeshUID = MeshUID;
	ClonedAttr->MaterialUID = MaterialUID;
	return ClonedAttr.Get();
}
//---------------------------------------------------------------------

}