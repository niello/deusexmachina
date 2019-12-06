#include "MeshData.h"
#include <Data/RAMData.h>

namespace Render
{
RTTI_CLASS_IMPL(Render::CMeshData, Resources::CResourceObject);

CMeshData::CMeshData() {}

CMeshData::~CMeshData()
{
	SAFE_FREE(pGroups);
}
//---------------------------------------------------------------------

void CMeshData::Destroy()
{
	pGroupLODMapping = nullptr;
	SAFE_FREE(pGroups);
}
//---------------------------------------------------------------------

void CMeshData::InitGroups(CPrimitiveGroup* pData, UPTR Count, UPTR SubMeshCount, UPTR LODCount, bool UseMapping, bool UpdateAABBs)
{
	_SubMeshCount = SubMeshCount;
	_LODCount = LODCount;

	if (UpdateAABBs)
	{
		const U32 VertexSize = GetVertexSize();
		for (UPTR i = 0; i < Count; ++i)
		{
			Render::CPrimitiveGroup& MeshGroup = pData[i];
			MeshGroup.AABB.BeginExtend();
			const U16* pIndex = static_cast<U16*>(IBData->GetPtr()) + MeshGroup.FirstIndex;
			for (U32 j = 0; j < MeshGroup.IndexCount; ++j)
			{
				const float* pVertex = static_cast<float*>(VBData->GetPtr()) + (pIndex[j] * VertexSize);
				MeshGroup.AABB.Extend(pVertex[0], pVertex[1], pVertex[2]);
			}
			MeshGroup.AABB.EndExtend();
		}
	}

	UPTR TotalSize;
	if (UseMapping)
	{
		_GroupCount = Count;
		TotalSize = sizeof(CPrimitiveGroup) * _GroupCount + sizeof(CPrimitiveGroup*) * SubMeshCount * LODCount;
	}
	else
	{
		n_assert(Count == 0 || Count == SubMeshCount * LODCount);
		_GroupCount = SubMeshCount * LODCount;
		TotalSize = sizeof(CPrimitiveGroup) * _GroupCount;
	}

	pGroups = (CPrimitiveGroup*)n_malloc(TotalSize);
	memcpy(pGroups, pData, TotalSize);
	if (UseMapping) pGroupLODMapping = (CPrimitiveGroup**)(pGroups + _GroupCount);
}
//---------------------------------------------------------------------

U32 CMeshData::GetVertexSize() const
{
	U32 Total = 0;
	for (const auto& Component : VertexFormat)
		Total += Component.GetSize();
	return Total;
}
//---------------------------------------------------------------------

bool CMeshData::UseRAMData()
{
	if (!VBData) FAIL;
	++RAMDataUseCounter;
	OK;
}
//---------------------------------------------------------------------

void CMeshData::ReleaseRAMData()
{
	--RAMDataUseCounter;
	if (!RAMDataUseCounter)
	{
		VBData.reset();
		IBData.reset();
	}
}
//---------------------------------------------------------------------

}