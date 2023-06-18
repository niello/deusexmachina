#include "SkyboxAttribute.h"
#include <Frame/View.h>
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

Render::PRenderable CSkyboxAttribute::CreateRenderable() const
{
	return std::make_unique<Render::CSkybox>();
}
//---------------------------------------------------------------------

// NB: LOD is not used for skyboxes
void CSkyboxAttribute::UpdateRenderable(CView& View, Render::IRenderable& Renderable) const
{
	auto pSkybox = static_cast<Render::CSkybox*>(&Renderable);

	if (!pSkybox->Mesh)
	{
		CStrID MeshUID("#Mesh_BoxCW");
		if (!View.GetGraphicsManager()->GetResourceManager()->FindResource(MeshUID))
		{
			// NB: CW box is created, because rendering is CCW, but front sides of polygons must be inside the skybox
			View.GetGraphicsManager()->GetResourceManager()->RegisterResource(MeshUID.CStr(), n_new(Resources::CMeshGeneratorBox(true)));
		}

		pSkybox->Mesh = View.GetGraphicsManager()->GetMesh(MeshUID);
	}

	// Initialize material
	if (!_MaterialUID)
	{
		if (pSkybox->Material)
		{
			pSkybox->Material = nullptr;
			pSkybox->ShaderTechIndex = INVALID_INDEX_T<U32>;
		}
	}
	else if (!pSkybox->Material || pSkybox->Material->GetUID() != _MaterialUID)
	{
		static const CStrID InputSet_Skybox("Skybox");

		pSkybox->Material = View.GetGraphicsManager()->GetMaterial(_MaterialUID);
		if (pSkybox->Material && pSkybox->Material->GetEffect())
			pSkybox->ShaderTechIndex = View.RegisterEffect(*pSkybox->Material->GetEffect(), InputSet_Skybox);
	}
}
//---------------------------------------------------------------------

bool CSkyboxAttribute::GetLocalAABB(CAABB& OutBox, UPTR LOD) const
{
	// Infinite size
	// FIXME: make return false mean an oversized object and detect invalid data otherwise, not by get AABB return value?
	OutBox = CAABB::Invalid;
	OK;
}
//---------------------------------------------------------------------

}
