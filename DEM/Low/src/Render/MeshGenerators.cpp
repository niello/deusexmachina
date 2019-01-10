#include "MeshGenerators.h"

#include <Render/Mesh.h>
#include <Render/VertexLayout.h>
#include <Render/GPUDriver.h>

namespace Resources
{
__ImplementClassNoFactory(Resources::CMeshGenerator, Resources::CResourceGenerator);
__ImplementClassNoFactory(Resources::CMeshGeneratorQuadPatch, Resources::CMeshGenerator);
__ImplementClassNoFactory(Resources::CMeshGeneratorSkybox, Resources::CMeshGenerator);

CMeshGenerator::CMeshGenerator() {}
CMeshGenerator::~CMeshGenerator() {}

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
	if (FrontClockWise)
	{
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
	}
	else
	{
		for (UPTR z = 0; z < QuadsPerEdge; ++z)
			for (UPTR x = 0; x < QuadsPerEdge; ++x)
			{
				*pIBCurr++ = (U16)(z * VerticesPerEdge + x);
				*pIBCurr++ = (U16)((z + 1) * VerticesPerEdge + x);
				*pIBCurr++ = (U16)(z * VerticesPerEdge + (x + 1));
				*pIBCurr++ = (U16)(z * VerticesPerEdge + (x + 1));
				*pIBCurr++ = (U16)((z + 1) * VerticesPerEdge + x);
				*pIBCurr++ = (U16)((z + 1) * VerticesPerEdge + (x + 1));
			}
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

PResourceObject CMeshGeneratorSkybox::Generate()
{
	if (GPU.IsNullPtr()) return NULL;

	Render::CVertexComponent VC;
	VC.Format = Render::VCFmt_Float32_3;
	VC.Semantic = Render::VCSem_Position;
	VC.Index = 0;
	VC.Stream = 0;
	VC.UserDefinedName = NULL;
	VC.Stream = 0;
	VC.OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
	VC.PerInstanceData = false;
	Render::PVertexLayout VertexLayout = GPU->CreateVertexLayout(&VC, 1);

	// Unit cube -0.5f .. +0.5f is too small for the near clip plane 0.1 in D3D9
	float VBData[24] =
	{
		-1.0f, -1.0f, -1.0f,
		+1.0f, -1.0f, -1.0f,
		-1.0f, +1.0f, -1.0f,
		+1.0f, +1.0f, -1.0f,
		-1.0f, -1.0f, +1.0f,
		+1.0f, -1.0f, +1.0f,
		-1.0f, +1.0f, +1.0f,
		+1.0f, +1.0f, +1.0f
	};
	Render::PVertexBuffer VB = GPU->CreateVertexBuffer(*VertexLayout.GetUnsafe(), 8, Render::Access_GPU_Read, &VBData);

	// Front side is inside a cube
	U16 IBDataCW[36] =
	{
		0, 2, 1, 1, 2, 3,	// Front	(-Z)
		1, 3, 5, 5, 3, 7,	// Right	(+X)
		5, 7, 4, 4, 7, 6,	// Back		(+Z)
		6, 7, 3, 6, 3, 2,	// Up		(+Y)
		6, 2, 0, 6, 0, 4,	// Left		(-X)
		4, 0, 5, 5, 0, 1	// Down		(-Y)
	};
	U16 IBDataCCW[36] =
	{
		0, 1, 2, 2, 1, 3,	// Front	(-Z)
		1, 5, 3, 3, 5, 7,	// Right	(+X)
		5, 4, 7, 7, 4, 6,	// Back		(+Z)
		6, 3, 7, 3, 6, 2,	// Up		(+Y)
		6, 0, 2, 0, 6, 4,	// Left		(-X)
		4, 5, 0, 0, 5, 1	// Down		(-Y)
	};
	Render::PIndexBuffer IB = GPU->CreateIndexBuffer(Render::Index_16, 36, Render::Access_GPU_Read, FrontClockWise ? &IBDataCW : &IBDataCCW);

	Render::CPrimitiveGroup Group;
	Group.Topology = Render::Prim_TriList;
	Group.FirstVertex = 0;
	Group.VertexCount = 8;
	Group.FirstIndex = 0;
	Group.IndexCount = 36;
	Group.AABB.Min.set(-0.5f, -0.5f, -0.5f);
	Group.AABB.Max.set(0.5f, 0.5f, 0.5f);

	Render::CMeshInitData InitData;
	InitData.pVertexBuffer = VB.GetUnsafe();
	InitData.pIndexBuffer = IB.GetUnsafe();
	InitData.pMeshGroupData = &Group;
	InitData.SubMeshCount = 1;
	InitData.LODCount = 1;
	InitData.RealGroupCount = 1;
	InitData.UseMapping = false;

	Render::PMesh Mesh = n_new(Render::CMesh);
	if (!Mesh->Create(InitData)) return NULL;
	return Mesh.GetUnsafe();
}
//---------------------------------------------------------------------

}