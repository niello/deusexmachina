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

bool CSkybox::LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count)
{
	for (UPTR j = 0; j < Count; ++j)
	{
		const uint32_t Code = DataReader.Read<uint32_t>();
		switch (Code)
		{
			case 'MTRL':
			{
				CString RsrcID = DataReader.Read<CString>();
				MaterialUID = CStrID(CString("Materials:") + RsrcID.CStr() + ".mtl"); //???replace ID by full URI on export?
				break;
			}
			default: FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

IRenderable* CSkybox::Clone()
{
	CSkybox* pCloned = n_new(CSkybox);
	pCloned->MaterialUID = MaterialUID;
	return pCloned;
}
//---------------------------------------------------------------------

bool CSkybox::ValidateResources(CGPUDriver* pGPU)
{
	//!!!double searching! Here and in GPU->CreateMesh!
	CStrID MeshUID("#Mesh_Skybox");
	if (!ResourceMgr->FindResource(MeshUID))
	{
		ResourceMgr->RegisterResource(MeshUID, n_new(Resources::CMeshGeneratorSkybox));
	}

	Material = pGPU ? pGPU->GetMaterial(MaterialUID) : nullptr;
	Mesh = pGPU ? pGPU->GetMesh(MeshUID) : nullptr;

	OK;
}
//---------------------------------------------------------------------

}