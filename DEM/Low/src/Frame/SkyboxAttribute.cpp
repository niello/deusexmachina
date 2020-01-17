#include "SkyboxAttribute.h"
#include <Frame/GraphicsResourceManager.h>
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
FACTORY_CLASS_IMPL(Frame::CSkyboxAttribute, 'SKBA', Frame::CRenderableAttribute);

bool CSkyboxAttribute::LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count)
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
			default: FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CSkyboxAttribute::Clone()
{
	PSkyboxAttribute ClonedAttr = n_new(CSkyboxAttribute());
	ClonedAttr->_MaterialUID = _MaterialUID;
	return ClonedAttr;
}
//---------------------------------------------------------------------

bool CSkyboxAttribute::ValidateGPUResources(CGraphicsResourceManager& ResMgr)
{
	CStrID MeshUID("#Mesh_BoxCW");
	if (!ResMgr.GetResourceManager()->FindResource(MeshUID))
	{
		// NB: CW box is created, because rendering is CCW, but front sides of polygons must be inside the skybox
		ResMgr.GetResourceManager()->RegisterResource(MeshUID, n_new(Resources::CMeshGeneratorBox(true)));
	}

	if (!Renderable) Renderable.reset(n_new(Render::CSkybox()));
	auto pSkybox = static_cast<Render::CSkybox*>(Renderable.get());

	pSkybox->Mesh = MeshUID ? ResMgr.GetMesh(MeshUID) : nullptr;
	pSkybox->Material = _MaterialUID ? ResMgr.GetMaterial(_MaterialUID) : nullptr;

	OK;
}
//---------------------------------------------------------------------

bool CSkyboxAttribute::GetLocalAABB(CAABB& OutBox, UPTR LOD) const
{
	// Infinite size
	// FIXME: make return false mean an oversized object and detect invalid data otherwise, not by get AABB return value?
	OutBox = CAABB::Empty;
	OK;
}
//---------------------------------------------------------------------

}