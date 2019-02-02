#include "MeshData.h"
#include <Data/RAMData.h>

namespace Render
{
__ImplementClassNoFactory(Render::CMeshData, Resources::CResourceObject);

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

void CMeshData::InitGroups(CPrimitiveGroup* pData, UPTR Count, UPTR SubMeshCount, UPTR LODCount, bool UseMapping)
{
	_SubMeshCount = SubMeshCount;
	_LODCount = LODCount;

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