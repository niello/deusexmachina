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

bool CMesh::Create(PMeshData Data, PVertexBuffer VertexBuffer, PIndexBuffer IndexBuffer, bool HoldRAMCopy)
{
	if (!VertexBuffer || !Data || !Data->GetSubMeshCount() || !Data->GetLODCount()) FAIL;

	MeshData = Data;
	VB = VertexBuffer;
	IB = IndexBuffer;

	HoldRAMBackingData = HoldRAMCopy;
	if (HoldRAMCopy) n_verify(MeshData->UseRAMData());

	OK;
}
//---------------------------------------------------------------------

void CMesh::Destroy()
{
	IB = nullptr;
	VB = nullptr;
	if (HoldRAMBackingData) MeshData->ReleaseRAMData();
	MeshData = nullptr;
}
//---------------------------------------------------------------------

const CPrimitiveGroup* CMesh::GetGroup(UPTR SubMeshIdx, UPTR LOD) const
{
	return MeshData ? MeshData->GetGroup(SubMeshIdx, LOD) : nullptr;
}
//---------------------------------------------------------------------

}