#include "MeshLoaderNVX2.h"

#include <Render/MeshData.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <Data/Buffer.h>

namespace Resources
{
#pragma pack(push, 1)
struct CNVX2Header
{
	U32 magic;
	U32 numGroups;
	U32 numVertices;
	U32 vertexWidth;
	U32 numIndices;
	U32 numEdges;
	U32 vertexComponentMask;
};

struct CNVX2Group
{
	U32 firstVertex;
	U32 numVertices;
	U32 firstTriangle;
	U32 numTriangles;
	U32 firstEdge;
	U32 numEdges;
};
#pragma pack(pop)

enum ENVX2VertexComponent
{
	Coord = (1 << 0),
	Normal = (1 << 1),
	Uv0 = (1 << 2),
	Uv1 = (1 << 3),
	Uv2 = (1 << 4),
	Uv3 = (1 << 5),
	Color = (1 << 6),
	Tangent = (1 << 7),
	Bitangent = (1 << 8),
	Weights = (1 << 9),
	JIndices = (1 << 10),
	Coord4 = (1 << 11),

	NumVertexComponents = 12,
	AllComponents = ((1 << NumVertexComponents) - 1)
};

static void SetupVertexComponents(U32 Mask, std::vector<Render::CVertexComponent>& Components)
{
	Render::CVertexComponent Cmp;
	Cmp.Stream = 0;
	Cmp.OffsetInVertex = Render::VertexComponentOffsetAuto;
	Cmp.UserDefinedName = nullptr;
	Cmp.PerInstanceData = false;

	if (Mask & Coord)
	{
		Cmp.Format = Render::EVertexComponentFormat::Float32_3;
		Cmp.Semantic = Render::EVertexComponentSemantic::Position;
		Cmp.Index = 0;
		Components.push_back(Cmp);
	}
	else if (Mask & Coord4)
	{
		Cmp.Format = Render::EVertexComponentFormat::Float32_4;
		Cmp.Semantic = Render::EVertexComponentSemantic::Position;
		Cmp.Index = 0;
		Components.push_back(Cmp);
	}

	if (Mask & Normal)
	{
		Cmp.Format = Render::EVertexComponentFormat::Float32_3;
		Cmp.Semantic = Render::EVertexComponentSemantic::Normal;
		Cmp.Index = 0;
		Components.push_back(Cmp);
	}

	if (Mask & Uv0)
	{
		Cmp.Format = Render::EVertexComponentFormat::Float32_2;
		Cmp.Semantic = Render::EVertexComponentSemantic::TexCoord;
		Cmp.Index = 0;
		Components.push_back(Cmp);
	}

	if (Mask & Uv1)
	{
		Cmp.Format = Render::EVertexComponentFormat::Float32_2;
		Cmp.Semantic = Render::EVertexComponentSemantic::TexCoord;
		Cmp.Index = 1;
		Components.push_back(Cmp);
	}

	if (Mask & Uv2)
	{
		Cmp.Format = Render::EVertexComponentFormat::Float32_2;
		Cmp.Semantic = Render::EVertexComponentSemantic::TexCoord;
		Cmp.Index = 2;
		Components.push_back(Cmp);
	}

	if (Mask & Uv3)
	{
		Cmp.Format = Render::EVertexComponentFormat::Float32_2;
		Cmp.Semantic = Render::EVertexComponentSemantic::TexCoord;
		Cmp.Index = 3;
		Components.push_back(Cmp);
	}

	if (Mask & Color)
	{
		Cmp.Format = Render::EVertexComponentFormat::Float32_4;
		Cmp.Semantic = Render::EVertexComponentSemantic::Color;
		Cmp.Index = 0;
		Components.push_back(Cmp);
	}

	if (Mask & Tangent)
	{
		Cmp.Format = Render::EVertexComponentFormat::Float32_3;
		Cmp.Semantic = Render::EVertexComponentSemantic::Tangent;
		Cmp.Index = 0;
		Components.push_back(Cmp);
	}

	if (Mask & Bitangent)
	{
		Cmp.Format = Render::EVertexComponentFormat::Float32_3;
		Cmp.Semantic = Render::EVertexComponentSemantic::Bitangent;
		Cmp.Index = 0;
		Components.push_back(Cmp);
	}

	if (Mask & Weights)
	{
		Cmp.Format = Render::EVertexComponentFormat::Float32_4;
		Cmp.Semantic = Render::EVertexComponentSemantic::BoneWeights;
		Cmp.Index = 0;
		Components.push_back(Cmp);
	}

	if (Mask & JIndices)
	{
		Cmp.Format = Render::EVertexComponentFormat::Float32_4;
		Cmp.Semantic = Render::EVertexComponentSemantic::BoneIndices;
		Cmp.Index = 0;
		Components.push_back(Cmp);
	}
}
//---------------------------------------------------------------------

const DEM::Core::CRTTI& CMeshLoaderNVX2::GetResultType() const
{
	return Render::CMeshData::RTTI;
}
//---------------------------------------------------------------------

DEM::Core::PObject CMeshLoaderNVX2::CreateResource(CStrID UID)
{
	const char* pOutSubId;
	IO::PStream Stream = _ResMgr.CreateResourceStream(UID.CStr(), pOutSubId, IO::SAP_SEQUENTIAL);
	if (!Stream || !Stream->IsOpened()) return nullptr;

	IO::CBinaryReader Reader(*Stream);

	// NVX2 is always TriList and Index16
	CNVX2Header Header;
	if (!Reader.Read(Header) || Header.magic != 'NVX2') return nullptr;
	Header.numIndices *= 3;

	Render::PMeshData MeshData = n_new(Render::CMeshData);
	MeshData->IndexType = Render::Index_16;
	MeshData->VertexCount = Header.numVertices;
	MeshData->IndexCount = Header.numIndices;

	std::vector<Render::CPrimitiveGroup> Groups;
	Groups.resize(Header.numGroups);
	for (U32 i = 0; i < Header.numGroups; ++i)
	{
		CNVX2Group Group;
		Reader.Read(Group);

		Render::CPrimitiveGroup& MeshGroup = Groups[i];
		MeshGroup.FirstVertex = Group.firstVertex;
		MeshGroup.VertexCount = Group.numVertices;
		MeshGroup.FirstIndex = Group.firstTriangle * 3;
		MeshGroup.IndexCount = Group.numTriangles * 3;
		MeshGroup.Topology = Render::Prim_TriList;
	}

	SetupVertexComponents(Header.vertexComponentMask, MeshData->VertexFormat);

	//!!!can map data through MMF instead!
	UPTR DataSize = Header.numVertices * Header.vertexWidth * sizeof(float);
	MeshData->VBData.reset(n_new(Data::CBufferMallocAligned(DataSize, 16)));
	Stream->Read(MeshData->VBData->GetPtr(), DataSize);

	//!!!can map data through MMF instead!
	DataSize = Header.numIndices * sizeof(U16);
	MeshData->IBData.reset(n_new(Data::CBufferMallocAligned(DataSize, 16)));
	Stream->Read(MeshData->IBData->GetPtr(), DataSize);

	MeshData->InitGroups(&Groups.front(), Groups.size(), Groups.size(), 1, false, true);

	return MeshData;
}
//---------------------------------------------------------------------

}
