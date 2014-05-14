#include "Mesh.h"

#include <Render/RenderServer.h>
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

	if (VertexBuffer->GetUsage() == Usage_Dynamic || (IndexBuffer && IndexBuffer->GetUsage() == Usage_Dynamic))
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