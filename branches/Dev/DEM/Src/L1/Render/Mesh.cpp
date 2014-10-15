#include "Mesh.h"

#include <Events/EventServer.h>
#include <Core/Factory.h>

namespace Render
{
__ImplementResourceClass(Render::CMesh, 'MESH', Resources::CResource);

bool CMesh::Setup(CVertexBuffer* VertexBuffer, CIndexBuffer* IndexBuffer, const CArray<CMeshGroup>& MeshGroups)
{
	if (!VertexBuffer || !MeshGroups.GetCount()) FAIL;
	VB = VertexBuffer;
	IB = IndexBuffer;
	Groups = MeshGroups;

	//!!!D3D9-specific, redesign!
	if (VertexBuffer->GetAccess().Is(GPU_Read | CPU_Write) || IndexBuffer->GetAccess().Is(GPU_Read | CPU_Write))
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

	VB = NULL;
	IB = NULL;
	Groups.Clear();
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