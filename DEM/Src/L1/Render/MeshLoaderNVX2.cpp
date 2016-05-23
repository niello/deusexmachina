#include "MeshLoaderNVX2.h"

#include <Render/Mesh.h>
#include <Render/GPUDriver.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Resources
{
__ImplementClass(Resources::CMeshLoaderNVX2, 'LMN2', Resources::CMeshLoader);

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
	Coord    = (1<<0),
	Normal   = (1<<1),
	Uv0      = (1<<2),
	Uv1      = (1<<3),
	Uv2      = (1<<4),
	Uv3      = (1<<5),
	Color    = (1<<6),
	Tangent  = (1<<7),
	Bitangent = (1<<8),
	Weights  = (1<<9),
	JIndices = (1<<10),
	Coord4   = (1<<11),

	NumVertexComponents = 12,
	AllComponents = ((1<<NumVertexComponents) - 1)
};

static void SetupVertexComponents(U32 Mask, CArray<Render::CVertexComponent>& Components)
{
	if (Mask & Coord)
	{
		Render::CVertexComponent& Cmp = *Components.Reserve(1);
		Cmp.Format = Render::VCFmt_Float32_3;
		Cmp.Semantic = Render::VCSem_Position;
		Cmp.Index = 0;
		Cmp.Stream = 0;
		Cmp.OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
		Cmp.UserDefinedName = NULL;
	}
	else if (Mask & Coord4)
	{
		Render::CVertexComponent& Cmp = *Components.Reserve(1);
		Cmp.Format = Render::VCFmt_Float32_4;
		Cmp.Semantic = Render::VCSem_Position;
		Cmp.Index = 0;
		Cmp.Stream = 0;
		Cmp.OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
		Cmp.UserDefinedName = NULL;
	}

	if (Mask & Normal)
	{
		Render::CVertexComponent& Cmp = *Components.Reserve(1);
		Cmp.Format = Render::VCFmt_Float32_3;
		Cmp.Semantic = Render::VCSem_Normal;
		Cmp.Index = 0;
		Cmp.Stream = 0;
		Cmp.OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
		Cmp.UserDefinedName = NULL;
	}

	if (Mask & Uv0)
	{
		Render::CVertexComponent& Cmp = *Components.Reserve(1);
		Cmp.Format = Render::VCFmt_Float32_2;
		Cmp.Semantic = Render::VCSem_TexCoord;
		Cmp.Index = 0;
		Cmp.Stream = 0;
		Cmp.OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
		Cmp.UserDefinedName = NULL;
	}

	if (Mask & Uv1)
	{
		Render::CVertexComponent& Cmp = *Components.Reserve(1);
		Cmp.Format = Render::VCFmt_Float32_2;
		Cmp.Semantic = Render::VCSem_TexCoord;
		Cmp.Index = 1;
		Cmp.Stream = 0;
		Cmp.OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
		Cmp.UserDefinedName = NULL;
	}

	if (Mask & Uv2)
	{
		Render::CVertexComponent& Cmp = *Components.Reserve(1);
		Cmp.Format = Render::VCFmt_Float32_2;
		Cmp.Semantic = Render::VCSem_TexCoord;
		Cmp.Index = 2;
		Cmp.Stream = 0;
		Cmp.OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
		Cmp.UserDefinedName = NULL;
	}

	if (Mask & Uv3)
	{
		Render::CVertexComponent& Cmp = *Components.Reserve(1);
		Cmp.Format = Render::VCFmt_Float32_2;
		Cmp.Semantic = Render::VCSem_TexCoord;
		Cmp.Index = 3;
		Cmp.Stream = 0;
		Cmp.OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
		Cmp.UserDefinedName = NULL;
	}

	if (Mask & Color)
	{
		Render::CVertexComponent& Cmp = *Components.Reserve(1);
		Cmp.Format = Render::VCFmt_Float32_4;
		Cmp.Semantic = Render::VCSem_Color;
		Cmp.Index = 0;
		Cmp.Stream = 0;
		Cmp.OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
		Cmp.UserDefinedName = NULL;
	}

	if (Mask & Tangent)
	{
		Render::CVertexComponent& Cmp = *Components.Reserve(1);
		Cmp.Format = Render::VCFmt_Float32_3;
		Cmp.Semantic = Render::VCSem_Tangent;
		Cmp.Index = 0;
		Cmp.Stream = 0;
		Cmp.OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
		Cmp.UserDefinedName = NULL;
	}

	if (Mask & Bitangent)
	{
		Render::CVertexComponent& Cmp = *Components.Reserve(1);
		Cmp.Format = Render::VCFmt_Float32_3;
		Cmp.Semantic = Render::VCSem_Bitangent;
		Cmp.Index = 0;
		Cmp.Stream = 0;
		Cmp.OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
		Cmp.UserDefinedName = NULL;
	}

	if (Mask & Weights)
	{
		Render::CVertexComponent& Cmp = *Components.Reserve(1);
		Cmp.Format = Render::VCFmt_Float32_4;
		Cmp.Semantic = Render::VCSem_BoneWeights;
		Cmp.Index = 0;
		Cmp.Stream = 0;
		Cmp.OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
		Cmp.UserDefinedName = NULL;
	}

	//???use ubyte4 for my geometry format?
	if (Mask & JIndices)
	{
		Render::CVertexComponent& Cmp = *Components.Reserve(1);
		Cmp.Format = Render::VCFmt_Float32_4;
		Cmp.Semantic = Render::VCSem_BoneIndices;
		Cmp.Index = 0;
		Cmp.Stream = 0;
		Cmp.OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
		Cmp.UserDefinedName = NULL;
	}
}
//---------------------------------------------------------------------

PResourceLoader CMeshLoaderNVX2::Clone()
{
	PMeshLoaderNVX2 NewLoader = n_new(CMeshLoaderNVX2);
	NewLoader->GPU = GPU;
	return NewLoader.GetUnsafe();
}
//---------------------------------------------------------------------

PResourceObject CMeshLoaderNVX2::Load(IO::CStream& Stream)
{
	IO::CBinaryReader Reader(Stream);

	// NVX2 is always TriList and Index16
	CNVX2Header Header;
	if (!Reader.Read(Header) || Header.magic != 'NVX2') return NULL;
	Header.numIndices *= 3;

	CArray<Render::CPrimitiveGroup> MeshGroups(Header.numGroups, 0);
	for (U32 i = 0; i < Header.numGroups; ++i)
	{
		CNVX2Group Group;
		Reader.Read(Group);

		Render::CPrimitiveGroup& MeshGroup = MeshGroups.At(i);
		MeshGroup.FirstVertex = Group.firstVertex;
		MeshGroup.VertexCount = Group.numVertices;
		MeshGroup.FirstIndex = Group.firstTriangle * 3;
		MeshGroup.IndexCount = Group.numTriangles * 3;
		MeshGroup.Topology = Render::Prim_TriList;
	}

	CArray<Render::CVertexComponent> Components;
	SetupVertexComponents(Header.vertexComponentMask, Components);
	Render::PVertexLayout VertexLayout = GPU->CreateVertexLayout(&Components.Front(), Components.GetCount());
	n_assert_dbg(VertexLayout->GetVertexSizeInBytes() == (Header.vertexWidth * sizeof(float)));

	//!!!map data through MMF instead!
	UPTR DataSize = Header.numVertices * Header.vertexWidth * sizeof(float);
	float* pVBData = (float*)n_malloc_aligned(DataSize, 16);
	Stream.Read(pVBData, DataSize);

	//!!!map data through MMF instead!
	DataSize = Header.numIndices * sizeof(U16);
	U16* pIBData = (U16*)n_malloc_aligned(DataSize, 16);
	Stream.Read(pIBData, DataSize);

	//!!!Now all VBs and IBs are not shared! later this may change!
	Render::PVertexBuffer VB = GPU->CreateVertexBuffer(*VertexLayout, Header.numVertices, Render::Access_GPU_Read, pVBData);
	Render::PIndexBuffer IB = GPU->CreateIndexBuffer(Render::Index_16, Header.numIndices, Render::Access_GPU_Read, pIBData);

	//!!!must be offline!
	for (UPTR i = 0; i < MeshGroups.GetCount(); ++i)
	{
		Render::CPrimitiveGroup& MeshGroup = MeshGroups[i];
		MeshGroup.AABB.BeginExtend();
		U16* pIndex = pIBData + MeshGroup.FirstIndex;
		for (U32 j = 0; j < MeshGroup.IndexCount; ++j)
		{
			float* pVertex = pVBData + (pIndex[j] * Header.vertexWidth);
			MeshGroup.AABB.Extend(pVertex[0], pVertex[1], pVertex[2]);
		}
		MeshGroup.AABB.EndExtend();
	}

	n_free_aligned(pVBData);
	n_free_aligned(pIBData);

	Render::CMeshInitData InitData;
	InitData.pVertexBuffer = VB;
	InitData.pIndexBuffer = IB;
	InitData.pMeshGroupData = &MeshGroups.Front();
	InitData.SubMeshCount = MeshGroups.GetCount();
	InitData.LODCount = 1;
	InitData.RealGroupCount = MeshGroups.GetCount();
	InitData.UseMapping = false;

	Render::PMesh Mesh = n_new(Render::CMesh);
	Mesh->Create(InitData);

	return Mesh.GetUnsafe();
}
//---------------------------------------------------------------------

}

////!!!this must be offline-processed! if it is hardware-dependent, need to research!
///*
////if (GetUsage() == WriteOnce)
//
//void
//nD3D9Mesh::OptimizeFaces(U16* indices, int numFaces, int numVertices)
//{
//    UPTR* pdwRemap = n_new_array(UPTR, numFaces);
//    D3DXOptimizeFaces(indices, numFaces, numVertices, FALSE, pdwRemap);
//
//    U16* dstIndices = n_new_array(U16, numFaces * 3);
//    n_assert(dstIndices);
//    memcpy(dstIndices, indices, numFaces * 6); // = 3 * sizeof(U16)
//
//    for (int i = 0; i < numFaces; ++i)
//    {
//        int newFace = (int) pdwRemap[i];
//        for (int j = 0; j < 3; ++j)
//            indices[newFace * 3 + j] = dstIndices[i * 3 + j];
//    }
//
//    n_delete_array(dstIndices);
//    n_delete_array(pdwRemap);
//}
//
//void
//nD3D9Mesh::OptimizeVertices(float* vertices, U16* indices, int numVertices, int numFaces)
//{
//    UPTR* pdwRemap = n_new_array(UPTR, numVertices);
//
//    D3DXOptimizeVertices(indices, numFaces, numVertices, FALSE, pdwRemap);
//
//    // remap vertices
//    float* dstVertices = n_new_array(float, numVertices * this->GetVertexWidth());
//    n_assert(dstVertices);
//    memcpy(dstVertices, vertices, numVertices * this->GetVertexWidth() * sizeof(float));
//
//    for (int i = 0; i < numVertices; ++i)
//    {
//        float* src = dstVertices + (i * this->GetVertexWidth());
//        float* dst = vertices + (pdwRemap[i] * this->GetVertexWidth());
//        memcpy(dst, src, this->GetVertexWidth() * sizeof(float));
//    }
//
//    // remap triangles
//    for (int faceIndex = 0; faceIndex < numFaces; ++faceIndex)
//    {
//        for (int index = 0; index < 3; ++index)
//        {
//            indices[faceIndex * 3 + index] = (U16) pdwRemap[indices[faceIndex * 3 + index]];
//        }
//    }
//
//    n_delete_array(dstVertices);
//    n_delete_array(pdwRemap);
//}
//
//*/