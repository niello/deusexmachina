#include "Mesh.h"

namespace Render
{

bool CMesh::Setup(CVertexBuffer* VertexBuffer, CIndexBuffer* IndexBuffer, const nArray<CMeshGroup>& MeshGroups)
{
	if (!VertexBuffer || !MeshGroups.Size()) FAIL;
	VB = VertexBuffer;
	IB = IndexBuffer;
	Groups = MeshGroups;
	State = Resources::Rsrc_Loaded;
	OK;
}
//---------------------------------------------------------------------

void CMesh::Unload()
{
	VB = NULL;
	IB = NULL;
	Groups.Clear();
	State = Resources::Rsrc_NotLoaded;
}
//---------------------------------------------------------------------

}