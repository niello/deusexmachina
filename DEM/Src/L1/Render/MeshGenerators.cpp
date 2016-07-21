#include "MeshGenerators.h"

#include <Render/Mesh.h>
#include <Render/VertexLayout.h>
#include <Render/GPUDriver.h>

namespace Resources
{
__ImplementClassNoFactory(Resources::CMeshGenerator, Resources::CResourceGenerator);
__ImplementClassNoFactory(Resources::CMeshGeneratorQuadPatch, Resources::CMeshGenerator);

const Core::CRTTI& CMeshGenerator::GetResultType() const
{
	return Render::CMesh::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CMeshGeneratorQuadPatch::Generate()
{
	if (GPU.IsNullPtr()) return NULL;

	float InvEdgeSize = 1.f / (float)QuadsPerEdge;
	UPTR VerticesPerEdge = QuadsPerEdge + 1;
	UPTR VertexCount = VerticesPerEdge * VerticesPerEdge;
	n_assert(VertexCount <= 65535); // because of 16-bit index buffer

	Render::CVertexComponent PatchVC;
	PatchVC.Format = Render::VCFmt_Float32_2;
	PatchVC.Semantic = Render::VCSem_Position;
	PatchVC.Index = 0;
	PatchVC.Stream = 0;
	PatchVC.UserDefinedName = NULL;
	PatchVC.Stream = 0;
	PatchVC.OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
	PatchVC.PerInstanceData = false;
	Render::PVertexLayout PatchVertexLayout = GPU->CreateVertexLayout(&PatchVC, 1);

	void* pVBData = n_malloc_aligned(VertexCount * 2 * sizeof(float), 16);
	float* pVBCurr = (float*)pVBData;
	for (UPTR z = 0; z < VerticesPerEdge; ++z)
		for (UPTR x = 0; x < VerticesPerEdge; ++x)
		{
			*pVBCurr++ = x * InvEdgeSize;
			*pVBCurr++ = z * InvEdgeSize;
		}
	Render::PVertexBuffer VB = GPU->CreateVertexBuffer(*PatchVertexLayout.GetUnsafe(), VertexCount, Render::Access_GPU_Read, pVBData);
	n_free_aligned(pVBData);

	//???use TriStrip?
	UPTR IndexCount = QuadsPerEdge * QuadsPerEdge * 6;

	void* pIBData = n_malloc_aligned(IndexCount * sizeof(U16), 16);
	U16* pIBCurr = (U16*)pIBData;
	for (UPTR z = 0; z < QuadsPerEdge; ++z)
		for (UPTR x = 0; x < QuadsPerEdge; ++x)
		{
			*pIBCurr++ = (U16)(z * VerticesPerEdge + x);
			*pIBCurr++ = (U16)(z * VerticesPerEdge + (x + 1));
			*pIBCurr++ = (U16)((z + 1) * VerticesPerEdge + x);
			*pIBCurr++ = (U16)(z * VerticesPerEdge + (x + 1));
			*pIBCurr++ = (U16)((z + 1) * VerticesPerEdge + (x + 1));
			*pIBCurr++ = (U16)((z + 1) * VerticesPerEdge + x);
		}
	Render::PIndexBuffer IB = GPU->CreateIndexBuffer(Render::Index_16, IndexCount, Render::Access_GPU_Read, pIBData);
	n_free_aligned(pIBData);

	Render::CPrimitiveGroup Group;
	Group.Topology = Render::Prim_TriList;
	Group.FirstVertex = 0;
	Group.VertexCount = VertexCount;
	Group.FirstIndex = 0;
	Group.IndexCount = IndexCount;
	Group.AABB.Min = vector3::Zero;
	Group.AABB.Max.set(1.f, 0.f, 1.f);

	Render::CMeshInitData InitData;
	InitData.pVertexBuffer = VB.GetUnsafe();
	InitData.pIndexBuffer = IB.GetUnsafe();
	InitData.pMeshGroupData = &Group;
	InitData.SubMeshCount = 1;
	InitData.LODCount = 1;
	InitData.RealGroupCount = 1;
	InitData.UseMapping = false;

	Render::PMesh Patch = n_new(Render::CMesh);
	if (!Patch->Create(InitData)) return NULL;
	return Patch.GetUnsafe();
}
//---------------------------------------------------------------------

}