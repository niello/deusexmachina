#include "MeshGenerators.h"
#include <Render/MeshData.h>
#include <Render/VertexComponent.h>
#include <Data/RAMData.h>

namespace Resources
{

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

	Render::CVertexComponent VC;
	VC.Format = Render::VCFmt_Float32_2;
	VC.Semantic = Render::VCSem_Position;
	VC.Index = 0;
	VC.Stream = 0;
	VC.UserDefinedName = nullptr;
	VC.Stream = 0;
	VC.OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
	VC.PerInstanceData = false;
	MeshData->VertexFormat.push_back(std::move(VC));

	MeshData->VBData.reset(n_new(Data::CRAMDataMallocAligned(VertexCount * MeshData->GetVertexSize(), 16)));
	float* pVBCurr = static_cast<float*>(MeshData->VBData->GetPtr());
	for (UPTR z = 0; z < VerticesPerEdge; ++z)
	{
		for (UPTR x = 0; x < VerticesPerEdge; ++x)
		{
			*pVBCurr++ = x * InvEdgeSize;
			*pVBCurr++ = z * InvEdgeSize;
		}
	}

	MeshData->IBData.reset(n_new(Data::CRAMDataMallocAligned(IndexCount * sizeof(U16), 16)));
	U16* pIBCurr = static_cast<U16*>(MeshData->IBData->GetPtr());
	if (_FrontClockWise)
	{
		for (UPTR z = 0; z < _QuadsPerEdge; ++z)
		{
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
	}
	else
	{
		for (UPTR z = 0; z < _QuadsPerEdge; ++z)
		{
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
	}

	Render::CPrimitiveGroup Group;
	Group.Topology = Render::Prim_TriList;
	Group.FirstVertex = 0;
	Group.VertexCount = VertexCount;
	Group.FirstIndex = 0;
	Group.IndexCount = IndexCount;
	Group.AABB.Min = vector3::Zero;
	Group.AABB.Max.set(1.f, 0.f, 1.f);

	MeshData->InitGroups(&Group, 1, 1, 1, false, false);

	return MeshData;
}
//---------------------------------------------------------------------

PResourceObject CMeshGeneratorBox::CreateResource(CStrID UID)
{
	Render::PMeshData MeshData = n_new(Render::CMeshData);
	MeshData->IndexType = Render::Index_16;
	MeshData->VertexCount = 24;
	MeshData->IndexCount = 36;

	Render::CVertexComponent VC;
	VC.Format = Render::VCFmt_Float32_3;
	VC.Semantic = Render::VCSem_Position;
	VC.Index = 0;
	VC.Stream = 0;
	VC.UserDefinedName = nullptr;
	VC.Stream = 0;
	VC.OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
	VC.PerInstanceData = false;
	MeshData->VertexFormat.push_back(std::move(VC));

	constexpr float VBData[24] =
	{
		-0.5f, -0.5f, -0.5f,
		+0.5f, -0.5f, -0.5f,
		-0.5f, +0.5f, -0.5f,
		+0.5f, +0.5f, -0.5f,
		-0.5f, -0.5f, +0.5f,
		+0.5f, -0.5f, +0.5f,
		-0.5f, +0.5f, +0.5f,
		+0.5f, +0.5f, +0.5f
	};
	const UPTR VertexDataSize = sizeof(VBData);
	MeshData->VBData.reset(n_new(Data::CRAMDataMallocAligned(VertexDataSize, 16)));
	memcpy(MeshData->VBData->GetPtr(), VBData, VertexDataSize);

	constexpr U16 IBDataCCW[36] =
	{
		0, 2, 1, 1, 2, 3,	// Front	(-Z)
		1, 3, 5, 5, 3, 7,	// Right	(+X)
		5, 7, 4, 4, 7, 6,	// Back		(+Z)
		6, 7, 3, 6, 3, 2,	// Up		(+Y)
		6, 2, 0, 6, 0, 4,	// Left		(-X)
		4, 0, 5, 5, 0, 1	// Down		(-Y)
	};
	constexpr U16 IBDataCW[36] =
	{
		0, 1, 2, 2, 1, 3,	// Front	(-Z)
		1, 5, 3, 3, 5, 7,	// Right	(+X)
		5, 4, 7, 7, 4, 6,	// Back		(+Z)
		6, 3, 7, 3, 6, 2,	// Up		(+Y)
		6, 0, 2, 0, 6, 4,	// Left		(-X)
		4, 5, 0, 0, 5, 1	// Down		(-Y)
	};
	const UPTR IndexDataSize = sizeof(IBDataCW);
	MeshData->IBData.reset(n_new(Data::CRAMDataMallocAligned(IndexDataSize, 16)));
	memcpy(MeshData->IBData->GetPtr(), _FrontClockWise ? IBDataCW : IBDataCCW, IndexDataSize);

	Render::CPrimitiveGroup Group;
	Group.Topology = Render::Prim_TriList;
	Group.FirstVertex = 0;
	Group.VertexCount = 8;
	Group.FirstIndex = 0;
	Group.IndexCount = 36;
	Group.AABB.Min.set(-0.5f, -0.5f, -0.5f);
	Group.AABB.Max.set(0.5f, 0.5f, 0.5f);

	MeshData->InitGroups(&Group, 1, 1, 1, false, false);

	return MeshData;
}
//---------------------------------------------------------------------

PResourceObject CMeshGeneratorSphere::CreateResource(CStrID UID)
{
	// Twice as much vertices on longitude result in more sphere-looking mesh
	const U16 MeridianCount = _RowCount * 2;

	Render::PMeshData MeshData = n_new(Render::CMeshData);
	MeshData->IndexType = Render::Index_16;
	MeshData->VertexCount = MeridianCount * (_RowCount - 1) + 2;
	MeshData->IndexCount = 6 * MeridianCount * (_RowCount - 1);

	Render::CVertexComponent VC;
	VC.Format = Render::VCFmt_Float32_3;
	VC.Semantic = Render::VCSem_Position;
	VC.Index = 0;
	VC.Stream = 0;
	VC.UserDefinedName = nullptr;
	VC.Stream = 0;
	VC.OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
	VC.PerInstanceData = false;
	MeshData->VertexFormat.push_back(std::move(VC));

	const float LatitudeStepInRadians = PI / static_cast<float>(_RowCount);
	const float LongitudeStepInRadians = (2.f * PI) / static_cast<float>(MeridianCount);
	constexpr float Radius = 0.5f;

	std::vector<vector3> Vertices;
	Vertices.reserve(MeshData->VertexCount);

	Vertices.push_back({ 0.f, Radius, 0.f }); // North pole
	for (UPTR i = 1; i < _RowCount; ++i)
	{
		const float Latitude = LatitudeStepInRadians * i;
		const float SinLat = std::sinf(Latitude);
		const float CosLat = std::cosf(Latitude);

		for (UPTR j = 0; j < MeridianCount; ++j)
		{
			const float Longitude = LongitudeStepInRadians * j;
			const float SinLon = std::sinf(Longitude);
			const float CosLon = std::cosf(Longitude);

			// See https://en.wikipedia.org/wiki/Spherical_coordinate_system
			// NB: catresian Z is up, but we use Y as up axis, so Y and Z are swapped
			Vertices.push_back({ Radius * SinLat * CosLon, Radius * CosLat, Radius * SinLat * SinLon });
		}
	}
	Vertices.push_back({ 0.f, -Radius, 0.f }); // South pole

	std::vector<U16> Indices;
	Indices.reserve(MeshData->IndexCount);

	// Top cap
	for (U16 i = 1; i < MeridianCount; ++i)
		Indices.insert(Indices.end(), { 0, i, static_cast<U16>(i + 1) });
	Indices.insert(Indices.end(), { 0, MeridianCount, 1 });

	// Rows
	for (U16 Row = 0; Row < (_RowCount - 2); ++Row)
	{
		const U16 Offset = Row * MeridianCount + 1;
		const U16 End = Offset + MeridianCount - 1;

		for (U16 i = Offset; i < End; ++i)
		{
			Indices.insert(Indices.end(),
				{
					static_cast<U16>(i),
					static_cast<U16>(i + MeridianCount),
					static_cast<U16>(i + MeridianCount + 1)
				});
			Indices.insert(Indices.end(),
				{
					static_cast<U16>(i),
					static_cast<U16>(i + MeridianCount + 1),
					static_cast<U16>(i + 1)
				});
		}

		// Tie up the strip
		Indices.insert(Indices.end(),
			{
				static_cast<U16>(End),
				static_cast<U16>(End + MeridianCount),
				static_cast<U16>(End + 1)
			});
		Indices.insert(Indices.end(),
			{
				static_cast<U16>(End),
				static_cast<U16>(End + 1),
				static_cast<U16>(Offset)
			});
	}

	// Bottom cap
	const U16 LastIndex = MeridianCount * (_RowCount - 1) + 1;
	const U16 BottomCapStart = LastIndex - MeridianCount;
	for (U16 i = BottomCapStart; i < LastIndex - 1; ++i)
		Indices.insert(Indices.end(), { LastIndex, static_cast<U16>(i + 1), i });
	Indices.insert(Indices.end(), { LastIndex, BottomCapStart, static_cast<U16>(LastIndex - 1) });

	const UPTR VertexDataSize = Vertices.size() * MeshData->GetVertexSize();
	MeshData->VBData.reset(n_new(Data::CRAMDataMallocAligned(VertexDataSize, 16)));
	memcpy(MeshData->VBData->GetPtr(), Vertices.data(), VertexDataSize);

	const UPTR IndexDataSize = Indices.size() * sizeof(U16);
	MeshData->IBData.reset(n_new(Data::CRAMDataMallocAligned(IndexDataSize, 16)));
	memcpy(MeshData->IBData->GetPtr(), Indices.data(), IndexDataSize);

	Render::CPrimitiveGroup Group;
	Group.Topology = Render::Prim_TriList;
	Group.FirstVertex = 0;
	Group.VertexCount = Vertices.size();
	Group.FirstIndex = 0;
	Group.IndexCount = Indices.size();
	Group.AABB.Min.set(-0.5f, -0.5f, -0.5f);
	Group.AABB.Max.set(0.5f, 0.5f, 0.5f);

	MeshData->InitGroups(&Group, 1, 1, 1, false, false);

	return MeshData;
}
//---------------------------------------------------------------------

}