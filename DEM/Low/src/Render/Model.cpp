#include "Model.h"

#include <Render/Mesh.h>
#include <Render/Material.h>
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <IO/PathUtils.h>
#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CModel, 'MODL', Render::IRenderable);

bool CModel::LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader)
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
		case 'JPLT':
		{
			U16 Count;
			if (!DataReader.Read(Count)) FAIL;
			BoneIndices.SetSize(Count);
			return DataReader.GetStream().Read(BoneIndices.GetPtr(), Count * sizeof(int)) == Count * sizeof(int);
		}
		case 'MESH':
		{
			//???!!!store the whole URI in a file?!
			CString MeshID = DataReader.Read<CString>();
			CStrID MeshURI = CStrID(CString("Meshes:") + MeshID.CStr() + ".nvx2");
			RMesh = ResourceMgr->RegisterResource(MeshURI);
			OK;
		}
		case 'MSGR':
		{
			return DataReader.Read(MeshGroupIndex);
		}
		default: FAIL;
	}
}
//---------------------------------------------------------------------

IRenderable* CModel::Clone()
{
	CModel* pCloned = n_new(CModel);
	pCloned->RMesh = RMesh;
	pCloned->RMaterial = RMaterial;
	pCloned->MeshGroupIndex = MeshGroupIndex;
	pCloned->BoneIndices.RawCopyFrom(BoneIndices.GetPtr(), BoneIndices.GetCount());
	return pCloned;
}
//---------------------------------------------------------------------

bool CModel::ValidateResources(CGPUDriver* pGPU)
{
	if (!RMesh->IsLoaded())
	{
		Resources::PResourceLoader Loader = RMesh->GetLoader();
		if (Loader.IsNullPtr())
			Loader = ResourceMgr->CreateDefaultLoaderFor<Render::CMesh>(PathUtils::GetExtension(RMesh->GetUID()));
		ResourceMgr->LoadResourceSync(*RMesh, *Loader);
		n_assert(RMesh->IsLoaded());
	}
	Mesh = RMesh->GetObject<Render::CMesh>();

	if (!RMaterial->IsLoaded())
	{
		Resources::PResourceLoader Loader = RMaterial->GetLoader();
		if (Loader.IsNullPtr())
			Loader = ResourceMgr->CreateDefaultLoaderFor<Render::CMaterial>(PathUtils::GetExtension(RMaterial->GetUID()));
		ResourceMgr->LoadResourceSync(*RMaterial, *Loader);
		n_assert(RMaterial->IsLoaded());
	}
	Material = RMaterial->GetObject<Render::CMaterial>();

	OK;
}
//---------------------------------------------------------------------

bool CModel::GetLocalAABB(CAABB& OutBox, UPTR LOD) const
{
	if (Mesh.IsNullPtr() || !Mesh->IsResourceValid()) FAIL;
	const Render::CPrimitiveGroup* pGroup = Mesh->GetGroup(MeshGroupIndex, LOD);
	if (!pGroup) FAIL;
	OutBox = pGroup->AABB;
	OK;
}
//---------------------------------------------------------------------

}