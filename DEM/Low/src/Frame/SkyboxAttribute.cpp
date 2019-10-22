#include "SkyboxAttribute.h"
#include <Frame/FrameResourceManager.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <Render/MeshGenerators.h>
#include <Render/Skybox.h>
#include <Render/Mesh.h>
#include <Render/Material.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Frame
{
__ImplementClass(Frame::CSkyboxAttribute, 'SKBA', Frame::CNodeAttrRenderable);

bool CSkyboxAttribute::LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count)
{
	if (!Renderable) Renderable.reset(n_new(Render::CSkybox));
	auto pSkybox = static_cast<Render::CSkybox*>(Renderable.get());

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
			default: FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool CSkyboxAttribute::ValidateResources(CGraphicsResourceManager& ResMgr)
{
	if (!Renderable) FAIL;

	//!!!double searching! Here and in ResMgr.CreateMesh!
	CStrID MeshUID("#Mesh_Skybox");
	if (!ResMgr.GetResourceManager()->FindResource(MeshUID))
	{
		ResMgr.GetResourceManager()->RegisterResource(MeshUID, n_new(Resources::CMeshGeneratorSkybox));
	}

	auto pSkybox = static_cast<Render::CSkybox*>(Renderable.get());
	pSkybox->Mesh = MeshUID ? ResMgr.GetMesh(MeshUID) : nullptr;
	pSkybox->Material = MaterialUID ? ResMgr.GetMaterial(MaterialUID) : nullptr;
	OK;
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CSkyboxAttribute::Clone()
{
	PSkyboxAttribute ClonedAttr = n_new(CSkyboxAttribute);
	if (Renderable) ClonedAttr->Renderable = std::move(Renderable->Clone());
	ClonedAttr->MaterialUID = MaterialUID;
	return ClonedAttr.Get();
}
//---------------------------------------------------------------------

}