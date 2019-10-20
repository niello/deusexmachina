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

bool CModel::LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count)
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
			case 'JPLT':
			{
				U16 Count;
				if (!DataReader.Read(Count)) FAIL;
				BoneIndices.SetSize(Count);
				if (DataReader.GetStream().Read(BoneIndices.GetPtr(), Count * sizeof(int)) != Count * sizeof(int)) FAIL;
				break;
			}
			case 'MESH':
			{
				//???!!!store the whole URI in a file?!
				CString MeshID = DataReader.Read<CString>();
				MeshUID = CStrID(CString("Meshes:") + MeshID.CStr() + ".nvx2");
				break;
			}
			case 'MSGR':
			{
				if (!DataReader.Read(MeshGroupIndex)) FAIL;
				break;
			}
			default: FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

IRenderable* CModel::Clone()
{
	CModel* pCloned = n_new(CModel);
	pCloned->MeshUID = MeshUID;
	pCloned->MaterialUID = MaterialUID;
	pCloned->MeshGroupIndex = MeshGroupIndex;
	pCloned->BoneIndices.RawCopyFrom(BoneIndices.GetPtr(), BoneIndices.GetCount());
	return pCloned;
}
//---------------------------------------------------------------------

bool CModel::ValidateResources(CGPUDriver* pGPU)
{
	Mesh = pGPU ? pGPU->GetMesh(MeshUID) : nullptr;
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