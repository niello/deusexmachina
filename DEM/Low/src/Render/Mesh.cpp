#include "Mesh.h"

#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CMesh, 'MESH', Resources::CResourceObject);

bool CMesh::Create(const CMeshInitData& InitData)
{
	n_assert(!pGroupLODMapping && !pGroups);

	if (!InitData.pVertexBuffer ||
		!InitData.SubMeshCount ||
		!InitData.LODCount ||
		(InitData.UseMapping && !InitData.RealGroupCount)) FAIL;

	VB = InitData.pVertexBuffer;
	IB = InitData.pIndexBuffer;

	SubMeshCount = InitData.SubMeshCount;
	LODCount = InitData.LODCount;

	UPTR TotalSize;
	if (InitData.UseMapping)
	{
		GroupCount = InitData.RealGroupCount;
		TotalSize = sizeof(CPrimitiveGroup) * GroupCount + sizeof(CPrimitiveGroup*) * SubMeshCount * LODCount;
	}
	else
	{
		n_assert(InitData.RealGroupCount == 0 || InitData.RealGroupCount == SubMeshCount * LODCount);
		GroupCount = SubMeshCount * LODCount;
		TotalSize = sizeof(CPrimitiveGroup) * GroupCount;
	}
	pGroups = (CPrimitiveGroup*)n_malloc(TotalSize);
	memcpy(pGroups, InitData.pMeshGroupData, TotalSize);
	if (InitData.UseMapping) pGroupLODMapping = (CPrimitiveGroup**)(pGroups + GroupCount);

	OK;
}
//---------------------------------------------------------------------

void CMesh::Destroy()
{
	n_free(pGroups);
	pGroups = NULL;
	pGroupLODMapping = NULL;

	IB = NULL;
	VB = NULL;
}
//---------------------------------------------------------------------

}