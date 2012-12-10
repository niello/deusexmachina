#include "Mesh.h"

namespace Render
{

bool CMesh::Setup(PVertexBuffer VertexBuffer, PIndexBuffer IndexBuffer, const nArray<CMeshGroup>& MeshGroups)
{
	if (!VertexBuffer.isvalid() || !MeshGroups.Size()) FAIL;
	VB = VertexBuffer;
	IB = IndexBuffer;
	Groups = MeshGroups;
	State = Resources::Rsrc_Loaded;
	OK;
}
//---------------------------------------------------------------------

}