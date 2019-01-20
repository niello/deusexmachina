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
			CStrID RUID = CStrID(CString("Materials:") + RsrcID.CStr() + ".mtl"); //???replace ID by full URI on export?
			RMaterial = ResourceMgr->RegisterResource<Render::CMaterial>(RUID);
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
	Material = RMaterial->ValidateObject<Render::CMaterial>();

	CStrID MeshUID("#Mesh_Skybox");
	Resources::PResource RMesh = ResourceMgr->FindResource(MeshUID);
	if (!RMesh)
	{
		Resources::PMeshGeneratorSkybox GenSkybox = n_new(Resources::CMeshGeneratorSkybox);
		GenSkybox->GPU = pGPU;
		ResourceMgr->RegisterResource(MeshUID, GenSkybox);
	}
	Mesh = RMesh->ValidateObject<CMesh>();

	OK;
}
//---------------------------------------------------------------------

}