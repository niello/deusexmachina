#include "Mesh.h"
#include <Render/MeshData.h>
#include <Core/Factory.h>

namespace Render
{
FACTORY_CLASS_IMPL(Render::CMesh, 'MESH', ::Core::CObject);

CMesh::CMesh() = default;

CMesh::~CMesh()
{
	Destroy();
}
//---------------------------------------------------------------------

bool CMesh::Create(CStrID UID, PMeshData Data, PVertexBuffer VertexBuffer, PIndexBuffer IndexBuffer, bool HoldRAMCopy)
{
	if (!VertexBuffer || !Data || !Data->GetSubMeshCount() || !Data->GetLODCount()) FAIL;

	_UID = UID;
	_MeshData = Data;
	_VB = VertexBuffer;
	_IB = IndexBuffer;

	_HoldRAMBackingData = HoldRAMCopy;
	if (HoldRAMCopy) n_verify(_MeshData->UseBuffer());

	OK;
}
//---------------------------------------------------------------------

void CMesh::Destroy()
{
	_IB = nullptr;
	_VB = nullptr;
	if (_HoldRAMBackingData) _MeshData->ReleaseBuffer();
	_MeshData = nullptr;
}
//---------------------------------------------------------------------

const CPrimitiveGroup* CMesh::GetGroup(UPTR SubMeshIdx, UPTR LOD) const
{
	return _MeshData ? _MeshData->GetGroup(SubMeshIdx, LOD) : nullptr;
}
//---------------------------------------------------------------------

}
