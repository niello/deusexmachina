#include "Mesh.h"

#include <Events/EventServer.h>
#include <Core/Factory.h>

namespace Render
{
__ImplementResourceClass(Render::CMesh, 'MESH', Resources::CResource);

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

	if (InitData.UseMapping)
	{
		GroupCount = InitData.RealGroupCount;
		DWORD MappingSize = sizeof(CMeshGroup*) * SubMeshCount * LODCount;
		DWORD TotalSize = MappingSize + sizeof(CMeshGroup) * GroupCount;
		pGroupLODMapping = (CMeshGroup**)n_malloc(TotalSize);
		pGroups = (CMeshGroup*)((char*)pGroupLODMapping + MappingSize);
		memcpy(pGroupLODMapping, InitData.pMeshGroupData, TotalSize);
	}
	else
	{
		GroupCount = (InitData.RealGroupCount == 0) ? SubMeshCount * LODCount : InitData.RealGroupCount;
		DWORD TotalSize = sizeof(CMeshGroup) * GroupCount;
		pGroups = (CMeshGroup*)n_malloc(TotalSize);
		memcpy(pGroups, InitData.pMeshGroupData, TotalSize);
	}

	//!!!D3D9-specific, redesign!
	if (VB->GetAccess().Is(GPU_Read | CPU_Write) || IB->GetAccess().Is(GPU_Read | CPU_Write))
	{
		SUBSCRIBE_PEVENT(OnRenderDeviceLost, CMesh, OnDeviceLost);
	}

	State = Resources::Rsrc_Loaded;
	OK;
}
//---------------------------------------------------------------------

void CMesh::Unload()
{
	UNSUBSCRIBE_EVENT(OnRenderDeviceLost);

	n_free(pGroupLODMapping ? (void*)pGroupLODMapping : (void*)pGroups);
	pGroups = NULL;
	pGroupLODMapping = NULL;

	IB = NULL;
	VB = NULL;
	State = Resources::Rsrc_NotLoaded;
}
//---------------------------------------------------------------------

bool CMesh::OnDeviceLost(const Events::CEventBase& Ev)
{
	if (IsLoaded()) Unload();
	OK;
}
//---------------------------------------------------------------------

}