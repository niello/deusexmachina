#include "Mesh.h"

#include <Events/EventServer.h>
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

	DWORD TotalSize;
	if (InitData.UseMapping)
	{
		GroupCount = InitData.RealGroupCount;
		TotalSize = sizeof(CMeshGroup) * GroupCount + sizeof(CMeshGroup*) * SubMeshCount * LODCount;
	}
	else
	{
		n_assert(InitData.RealGroupCount == 0 || InitData.RealGroupCount == SubMeshCount * LODCount);
		GroupCount = SubMeshCount * LODCount;
		TotalSize = sizeof(CMeshGroup) * GroupCount;
	}
	pGroups = (CMeshGroup*)n_malloc(TotalSize);
	memcpy(pGroups, InitData.pMeshGroupData, TotalSize);
	if (InitData.UseMapping) pGroupLODMapping = (CMeshGroup**)(pGroups + GroupCount);

	OK;
}
//---------------------------------------------------------------------

void CMesh::Unload()
{
	n_free(pGroups);
	pGroups = NULL;
	pGroupLODMapping = NULL;

	IB = NULL;
	VB = NULL;
}
//---------------------------------------------------------------------

}