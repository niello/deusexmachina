#include "Mesh.h"
#include <Render/MeshData.h>
#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CMesh, 'MESH', Resources::CResourceObject);

CMesh::CMesh() {}

CMesh::~CMesh()
{
	Destroy();
}
//---------------------------------------------------------------------

bool CMesh::Create(PMeshData Data, PVertexBuffer VertexBuffer, PIndexBuffer IndexBuffer)
{
	if (!VertexBuffer || !Data || !Data->GetSubMeshCount() || !Data->GetLODCount()) FAIL;

	MeshData = Data;
	VB = VertexBuffer;
	IB = IndexBuffer;

	OK;
}
//---------------------------------------------------------------------

void CMesh::Destroy()
{
	IB = nullptr;
	VB = nullptr;
	MeshData = nullptr;
}
//---------------------------------------------------------------------

const CPrimitiveGroup* CMesh::GetGroup(UPTR SubMeshIdx, UPTR LOD) const
{
	return MeshData ? MeshData->GetGroup(SubMeshIdx, LOD) : nullptr;
}
//---------------------------------------------------------------------

}