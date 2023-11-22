#include "MeshData.h"
#include <Data/Buffer.h>

namespace Render
{

CMeshData::CMeshData() = default;

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
	pGroupLODMapping = nullptr;
	SAFE_FREE(pGroups);

	_SubMeshCount = SubMeshCount;
	_LODCount = LODCount;

	if (UpdateAABBs)
	{
		UPTR VertexSize = 0;
		UPTR PositionOffset = static_cast<UPTR>(-1);
		for (const auto& Component : VertexFormat)
		{
			if (Component.Semantic == EVertexComponentSemantic::Position &&
				Component.Format == EVertexComponentFormat::Float32_3)
				PositionOffset = VertexSize;

			VertexSize += Component.GetSize();
		}

		if (PositionOffset < VertexSize)
		{
			auto pPositionData = static_cast<const char*>(VBData->GetPtr()) + PositionOffset;
			for (UPTR i = 0; i < Count; ++i)
			{
				Render::CPrimitiveGroup& MeshGroup = pData[i];

				if (!MeshGroup.IndexCount)
				{
					MeshGroup.AABB = Math::EmptyAABB();
					continue;
				}

				rtm::vector4f Min = rtm::vector_set(FLT_MAX, FLT_MAX, FLT_MAX, 0.f);
				rtm::vector4f Max = rtm::vector_set(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0.f);

				if (IndexType == Index_16)
				{
					const U16* pIndex = static_cast<U16*>(IBData->GetPtr()) + MeshGroup.FirstIndex;
					const U16* pIndexEnd = pIndex + MeshGroup.IndexCount;
					for (; pIndex < pIndexEnd; ++pIndex)
					{
						const rtm::vector4f Vertex = rtm::vector_load3(reinterpret_cast<const float*>(pPositionData + (*pIndex) * VertexSize));
						Min = rtm::vector_min(Min, Vertex);
						Max = rtm::vector_max(Max, Vertex);
					}
				}
				else
				{
					const U32* pIndex = static_cast<U32*>(IBData->GetPtr()) + MeshGroup.FirstIndex;
					const U32* pIndexEnd = pIndex + MeshGroup.IndexCount;
					for (; pIndex < pIndexEnd; ++pIndex)
					{
						const rtm::vector4f Vertex = rtm::vector_load3(reinterpret_cast<const float*>(pPositionData + (*pIndex) * VertexSize));
						Min = rtm::vector_min(Min, Vertex);
						Max = rtm::vector_max(Max, Vertex);
					}
				}

				MeshGroup.AABB = Math::AABBFromMinMax(Min, Max);
			}
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

	pGroups = static_cast<CPrimitiveGroup*>(n_malloc(TotalSize));
	std::memcpy(pGroups, pData, TotalSize);
	if (UseMapping) pGroupLODMapping = (CPrimitiveGroup**)(pGroups + _GroupCount);

	for (U8 i = 0; i < Count; ++i)
		pGroups[i].IndexInMesh = i;
}
//---------------------------------------------------------------------

UPTR CMeshData::GetVertexSize() const
{
	UPTR Total = 0;
	for (const auto& Component : VertexFormat)
		Total += Component.GetSize();
	return Total;
}
//---------------------------------------------------------------------

bool CMeshData::UseBuffer()
{
	if (!VBData) FAIL;
	++BufferUseCounter;
	OK;
}
//---------------------------------------------------------------------

void CMeshData::ReleaseBuffer()
{
	--BufferUseCounter;
	if (!BufferUseCounter)
	{
		VBData.reset();
		IBData.reset();
	}
}
//---------------------------------------------------------------------

}
