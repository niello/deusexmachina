#include "Skybox.h"

#include <Render/Material.h>
#include <Render/MeshGenerators.h>
#include <Render/GPUDriver.h>
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
#include <Resources/ResourceCreator.h>
#include <IO/BinaryReader.h>
#include <IO/PathUtils.h>
#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CSkybox, 'SKBX', Render::IRenderable);

bool CSkybox::LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader)
{
	switch (FourCC.Code)
	{
		case 'MTRL':
		{
			CString RsrcID = DataReader.Read<CString>();
			CStrID RsrcURI = CStrID(CString("Materials:") + RsrcID.CStr() + ".mtl"); //???replace ID by full URI on export?
			RMaterial = ResourceMgr->RegisterResource(RsrcURI);
			OK;
		}
		default: FAIL;
	}
}
//---------------------------------------------------------------------

IRenderable* CSkybox::Clone()
{
	CSkybox* pCloned = n_new(CSkybox);
	pCloned->RMaterial = RMaterial;
	return pCloned;
}
//---------------------------------------------------------------------

bool CSkybox::ValidateResources(CGPUDriver* pGPU)
{
	if (!RMaterial->IsLoaded())
	{
		Resources::PResourceLoader Loader = RMaterial->GetLoader();
		if (Loader.IsNullPtr())
			Loader = ResourceMgr->CreateDefaultLoaderFor<Render::CMaterial>(PathUtils::GetExtension(RMaterial->GetUID()));
		ResourceMgr->LoadResourceSync(*RMaterial, *Loader);
		n_assert(RMaterial->IsLoaded());
	}
	Material = RMaterial->GetObject<Render::CMaterial>();

	Resources::PResource RMesh = ResourceMgr->RegisterResource("Mesh_Skybox");
	if (!RMesh->IsLoaded())
	{
		Resources::PResourceCreator Gen = RMesh->GetGenerator();
		if (Gen.IsNullPtr())
		{
			Resources::PMeshGeneratorSkybox GenSkybox = n_new(Resources::CMeshGeneratorSkybox);
			GenSkybox->GPU = pGPU;
			Gen = GenSkybox.Get();
		}
		ResourceMgr->GenerateResourceSync(*RMesh, *Gen);
		n_assert(RMesh->IsLoaded());
	}
	Mesh = RMesh->GetObject<CMesh>();

	OK;
}
//---------------------------------------------------------------------

}