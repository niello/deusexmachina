#include "MeshGenerators.h"

#include <Render/MeshData.h>
#include <Render/VertexComponent.h>
#include <Data/RAMData.h>

namespace Resources
{
CMeshGenerator::CMeshGenerator() {}
CMeshGenerator::~CMeshGenerator() {}

const Core::CRTTI& CMeshGenerator::GetResultType() const
{
	return Render::CMeshData::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CMeshGeneratorQuadPatch::CreateResource(CStrID UID)
{
	const float InvEdgeSize = 1.f / static_cast<float>(_QuadsPerEdge);
	const UPTR VerticesPerEdge = _QuadsPerEdge + 1;
	const UPTR VertexCount = VerticesPerEdge * VerticesPerEdge;
	n_assert(VertexCount <= 65535); // because of 16-bit index buffer

	const UPTR IndexCount = _QuadsPerEdge * _QuadsPerEdge * 6; //???use TriStrip?

	Render::PMeshData MeshData = n_new(Render::CMeshData);
	MeshData->IndexType = Render::Index_16;
	MeshData->VertexCount = VertexCount;
	MeshData->IndexCount = IndexCount;

	auto& VC = *MeshData->VertexFormat.Reserve(1);
	VC.Format = Render::VCFmt_Float32_2;
	VC.Semantic = Render::VCSem_Position;
	VC.Index = 0;
	VC.Stream = 0;
	VC.UserDefinedName = NULL;
	VC.Stream = 0;
	VC.OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
	VC.PerInstanceData = false;

	const UPTR VertexSize = VC.GetSize();

	MeshData->VBData.reset(n_new(Data::CRAMDataMallocAligned(VertexCount * VertexSize * sizeof(float), 16)));
	float* pVBCurr = static_cast<float*>(MeshData->VBData->GetPtr());
	for (UPTR z = 0; z < VerticesPerEdge; ++z)
		for (UPTR x = 0; x < VerticesPerEdge; ++x)
		{
			*pVBCurr++ = x * InvEdgeSize;
			*pVBCurr++ = z * InvEdgeSize;
		}


	MeshData->IBData.reset(n_new(Data::CRAMDataMallocAligned(IndexCount * sizeof(U16), 16)));
	U16* pIBCurr = static_cast<U16*>(MeshData->IBData->GetPtr());
	if (FrontClockWise)
	{
		for (UPTR z = 0; z < _QuadsPerEdge; ++z)
			for (UPTR x = 0; x < _QuadsPerEdge; ++x)
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
		for (UPTR z = 0; z < _QuadsPerEdge; ++z)
			for (UPTR x = 0; x < _QuadsPerEdge; ++x)
			{
				*pIBCurr++ = (U16)(z * VerticesPerEdge + x);
				*pIBCurr++ = (U16)((z + 1) * VerticesPerEdge + x);
				*pIBCurr++ = (U16)(z * VerticesPerEdge + (x + 1));
				*pIBCurr++ = (U16)(z * VerticesPerEdge + (x + 1));
				*pIBCurr++ = (U16)((z + 1) * VerticesPerEdge + x);
				*pIBCurr++ = (U16)((z + 1) * VerticesPerEdge + (x + 1));
			}
	}

	Render::CPrimitiveGroup Group;
	Group.Topology = Render::Prim_TriList;
	Group.FirstVertex = 0;
	Group.VertexCount = VertexCount;
	Group.FirstIndex = 0;
	Group.IndexCount = IndexCount;
	Group.AABB.Min = vector3::Zero;
	Group.AABB.Max.set(1.f, 0.f, 1.f);

	MeshData->InitGroups(&Group, 1, 1, 1, false);

	return MeshData;
}
//---------------------------------------------------------------------

PResourceObject CMeshGeneratorSkybox::CreateResource(CStrID UID)
{
	Render::PMeshData MeshData = n_new(Render::CMeshData);

	auto& VC = *MeshData->VertexFormat.Reserve(1);
	VC.Format = Render::VCFmt_Float32_3;
	VC.Semantic = Render::VCSem_Position;
	VC.Index = 0;
	VC.Stream = 0;
	VC.UserDefinedName = NULL;
	VC.Stream = 0;
	VC.OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
	VC.PerInstanceData = false;

	const UPTR VertexSize = VC.GetSize();

	// Unit cube -0.5f .. +0.5f is too small for the near clip plane 0.1 in D3D9
	constexpr float VBData[24] =
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
	const UPTR VertexDataSize = sizeof_array(VBData) * VertexSize * sizeof(float);
	MeshData->VBData.reset(n_new(Data::CRAMDataMallocAligned(VertexDataSize, 16)));
	memcpy(MeshData->VBData->GetPtr(), VBData, VertexDataSize);

	// Front side is inside a cube
	constexpr U16 IBDataCW[36] =
	{
		0, 2, 1, 1, 2, 3,	// Front	(-Z)
		1, 3, 5, 5, 3, 7,	// Right	(+X)
		5, 7, 4, 4, 7, 6,	// Back		(+Z)
		6, 7, 3, 6, 3, 2,	// Up		(+Y)
		6, 2, 0, 6, 0, 4,	// Left		(-X)
		4, 0, 5, 5, 0, 1	// Down		(-Y)
	};
	constexpr U16 IBDataCCW[36] =
	{
		0, 1, 2, 2, 1, 3,	// Front	(-Z)
		1, 5, 3, 3, 5, 7,	// Right	(+X)
		5, 4, 7, 7, 4, 6,	// Back		(+Z)
		6, 3, 7, 3, 6, 2,	// Up		(+Y)
		6, 0, 2, 0, 6, 4,	// Left		(-X)
		4, 5, 0, 0, 5, 1	// Down		(-Y)
	};
	const UPTR IndexDataSize = sizeof_array(IBDataCW) * sizeof(U16);
	MeshData->IBData.reset(n_new(Data::CRAMDataMallocAligned(IndexDataSize, 16)));
	memcpy(MeshData->IBData->GetPtr(), FrontClockWise ? IBDataCW : IBDataCCW, IndexDataSize);

	Render::CPrimitiveGroup Group;
	Group.Topology = Render::Prim_TriList;
	Group.FirstVertex = 0;
	Group.VertexCount = 8;
	Group.FirstIndex = 0;
	Group.IndexCount = 36;
	Group.AABB.Min.set(-0.5f, -0.5f, -0.5f);
	Group.AABB.Max.set(0.5f, 0.5f, 0.5f);

	MeshData->InitGroups(&Group, 1, 1, 1, false);

	return MeshData;
}
//---------------------------------------------------------------------

}