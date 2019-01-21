#include "Model.h"

#include <Render/Mesh.h>
#include <Render/Material.h>
#include <Render/GPUDriver.h>
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
			MaterialUID = CStrID(CString("Materials:") + RsrcID.CStr() + ".mtl"); //???replace ID by full URI on export?
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
			CStrID RUID = CStrID(CString("Meshes:") + MeshID.CStr() + ".nvx2");
			RMesh = ResourceMgr->RegisterResource<Render::CMesh>(RUID);			
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
	pCloned->MaterialUID = MaterialUID;
	pCloned->MeshGroupIndex = MeshGroupIndex;
	pCloned->BoneIndices.RawCopyFrom(BoneIndices.GetPtr(), BoneIndices.GetCount());
	return pCloned;
}
//---------------------------------------------------------------------

bool CModel::ValidateResources(CGPUDriver* pGPU)
{
	Mesh = RMesh->ValidateObject<Render::CMesh>();
	Material = pGPU ? pGPU->GetMaterial(MaterialUID) : nullptr;
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