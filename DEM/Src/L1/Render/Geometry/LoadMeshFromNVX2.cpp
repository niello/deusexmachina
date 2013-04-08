// Loads mesh vertices, indices and primitive groups from .nvx2 file
// Use function declaration instead of header file where you want to call this loader.

#include <Render/RenderServer.h>
#include <Data/BinaryReader.h>
#include <Data/Streams/FileStream.h>

namespace Render
{
#pragma pack(push, 1)
struct CNVX2Header
{
	uint magic;
	uint numGroups;
	uint numVertices;
	uint vertexWidth;
	uint numIndices;
	uint numEdges;
	uint vertexComponentMask;
};

struct CNVX2Group
{
	uint firstVertex;
	uint numVertices;
	uint firstTriangle;
	uint numTriangles;
	uint firstEdge;
	uint numEdges;
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
	Binormal = (1<<8),
	Weights  = (1<<9),
	JIndices = (1<<10),
	Coord4   = (1<<11),

	NumVertexComponents = 12,
	AllComponents = ((1<<NumVertexComponents) - 1)
};

static void SetupVertexComponents(uint Mask, nArray<CVertexComponent>& Components);

//!!!can write/use as default mapped reading, when file is read as its whole
//so there will be two loaders, memory and stream. Memory is faster, stream allows to load very big files.
//!!!usage & access!
bool LoadMeshFromNVX2(Data::CStream& In, PMesh OutMesh)
{
	if (!OutMesh.isvalid()) FAIL;

	Data::CBinaryReader Reader(In);

	// NVX2 is always TriList and Index16
	CNVX2Header Header;
	Reader.Read(Header);
	if (Header.magic != 'NVX2') FAIL;
	Header.numIndices *= 3;

	nArray<CMeshGroup> MeshGroups(Header.numGroups, 0);
	for (uint i = 0; i < Header.numGroups; ++i)
	{
		CNVX2Group Group;
		Reader.Read(Group);

		CMeshGroup& MeshGroup = MeshGroups.At(i);
		MeshGroup.FirstVertex = Group.firstVertex;
		MeshGroup.VertexCount = Group.numVertices;
		MeshGroup.FirstIndex = Group.firstTriangle * 3;
		MeshGroup.IndexCount = Group.numTriangles * 3;
		MeshGroup.Topology = TriList;
	}

	nArray<CVertexComponent> Components;
	SetupVertexComponents(Header.vertexComponentMask, Components);
	PVertexLayout VertexLayout = RenderSrv->GetVertexLayout(Components);
	n_assert_dbg(VertexLayout->GetVertexSize() == (Header.vertexWidth * sizeof(float)));

	DWORD VBBegin = In.GetPosition();

	//!!!Now all VBs and IBs are not shared! later this may change!
	PVertexBuffer VB = n_new(CVertexBuffer);
	if (!VB->Create(VertexLayout, Header.numVertices, Usage_Immutable, CPU_NoAccess)) FAIL;
	void* pData = VB->Map(Map_Setup);
	In.Read(pData, Header.numVertices * Header.vertexWidth * sizeof(float));
	VB->Unmap();

	PIndexBuffer IB = n_new(CIndexBuffer);
	if (!IB->Create(CIndexBuffer::Index16, Header.numIndices, Usage_Immutable, CPU_NoAccess)) FAIL;
	pData = IB->Map(Map_Setup);
	In.Read(pData, Header.numIndices * sizeof(short));
	IB->Unmap();

//!!!OFFLINE+
//!!!must be offline!
	In.Seek(VBBegin, Data::SSO_BEGIN);

	DWORD ElmCount = Header.numVertices * Header.vertexWidth;
	float* pVBData = n_new_array(float, ElmCount);
	In.Read(pVBData, ElmCount * sizeof(float));

	ushort* pIBData = n_new_array(ushort, Header.numIndices);
	In.Read(pIBData, Header.numIndices * sizeof(short));

	for (int i = 0; i < MeshGroups.Size(); ++i)
	{
		CMeshGroup& MeshGroup = MeshGroups[i];
		MeshGroup.AABB.begin_extend();
		ushort* pIndex = pIBData + MeshGroup.FirstIndex;
		for (uint j = 0; j < MeshGroup.IndexCount; ++j)
		{
			float* pVertex = pVBData + (pIndex[j] * Header.vertexWidth);
			MeshGroup.AABB.extend(pVertex[0], pVertex[1], pVertex[2]);
		}
	}

	n_delete_array(pVBData);
	n_delete_array(pIBData);
//!!!OFFLINE-

	return OutMesh->Setup(VB, IB, MeshGroups);
}
//---------------------------------------------------------------------

//!!!usage & access!
bool LoadMeshFromNVX2(const nString& FileName, PMesh OutMesh)
{
	Data::CFileStream File;
	return File.Open(FileName, Data::SAM_READ, Data::SAP_SEQUENTIAL) &&
		LoadMeshFromNVX2(File, OutMesh);
}
//---------------------------------------------------------------------

static void SetupVertexComponents(uint Mask, nArray<CVertexComponent>& Components)
{
	if (Mask & Coord)
	{
		CVertexComponent& Cmp = *Components.Reserve(1);
		Cmp.Format = CVertexComponent::Float3;
		Cmp.Semantic = CVertexComponent::Position;
		Cmp.Index = 0;
		Cmp.Stream = 0;
	}
	else if (Mask & Coord4)
	{
		CVertexComponent& Cmp = *Components.Reserve(1);
		Cmp.Format = CVertexComponent::Float4;
		Cmp.Semantic = CVertexComponent::Position;
		Cmp.Index = 0;
		Cmp.Stream = 0;
	}

	if (Mask & Normal)
	{
		CVertexComponent& Cmp = *Components.Reserve(1);
		Cmp.Format = CVertexComponent::Float3;
		Cmp.Semantic = CVertexComponent::Normal;
		Cmp.Index = 0;
		Cmp.Stream = 0;
	}

	if (Mask & Uv0)
	{
		CVertexComponent& Cmp = *Components.Reserve(1);
		Cmp.Format = CVertexComponent::Float2;
		Cmp.Semantic = CVertexComponent::TexCoord;
		Cmp.Index = 0;
		Cmp.Stream = 0;
	}

	if (Mask & Uv1)
	{
		CVertexComponent& Cmp = *Components.Reserve(1);
		Cmp.Format = CVertexComponent::Float2;
		Cmp.Semantic = CVertexComponent::TexCoord;
		Cmp.Index = 1;
		Cmp.Stream = 0;
	}

	if (Mask & Uv2)
	{
		CVertexComponent& Cmp = *Components.Reserve(1);
		Cmp.Format = CVertexComponent::Float2;
		Cmp.Semantic = CVertexComponent::TexCoord;
		Cmp.Index = 2;
		Cmp.Stream = 0;
	}

	if (Mask & Uv3)
	{
		CVertexComponent& Cmp = *Components.Reserve(1);
		Cmp.Format = CVertexComponent::Float2;
		Cmp.Semantic = CVertexComponent::TexCoord;
		Cmp.Index = 3;
		Cmp.Stream = 0;
	}

	if (Mask & Color)
	{
		CVertexComponent& Cmp = *Components.Reserve(1);
		Cmp.Format = CVertexComponent::Float4;
		Cmp.Semantic = CVertexComponent::Color;
		Cmp.Index = 0;
		Cmp.Stream = 0;
	}

	if (Mask & Tangent)
	{
		CVertexComponent& Cmp = *Components.Reserve(1);
		Cmp.Format = CVertexComponent::Float3;
		Cmp.Semantic = CVertexComponent::Tangent;
		Cmp.Index = 0;
		Cmp.Stream = 0;
	}

	if (Mask & Binormal)
	{
		CVertexComponent& Cmp = *Components.Reserve(1);
		Cmp.Format = CVertexComponent::Float3;
		Cmp.Semantic = CVertexComponent::Binormal;
		Cmp.Index = 0;
		Cmp.Stream = 0;
	}

	if (Mask & Weights)
	{
		CVertexComponent& Cmp = *Components.Reserve(1);
		Cmp.Format = CVertexComponent::Float4;
		Cmp.Semantic = CVertexComponent::BoneWeights;
		Cmp.Index = 0;
		Cmp.Stream = 0;
	}

	//???use ubyte4 for my geometry?
	if (Mask & JIndices)
	{
		CVertexComponent& Cmp = *Components.Reserve(1);
		Cmp.Format = CVertexComponent::Float4;
		Cmp.Semantic = CVertexComponent::BoneIndices;
		Cmp.Index = 0;
		Cmp.Stream = 0;
	}
}
//---------------------------------------------------------------------

//!!!this must be offline-processed! if it is hardware-dependent, need to research!
/*
//if (GetUsage() == WriteOnce)

void
nD3D9Mesh::OptimizeFaces(ushort* indices, int numFaces, int numVertices)
{
    DWORD* pdwRemap = n_new_array(DWORD, numFaces);
    D3DXOptimizeFaces(indices, numFaces, numVertices, FALSE, pdwRemap);

    ushort* dstIndices = n_new_array(ushort, numFaces * 3);
    n_assert(dstIndices);
    memcpy(dstIndices, indices, numFaces * 6); // = 3 * sizeof(ushort)

    for (int i = 0; i < numFaces; ++i)
    {
        int newFace = (int) pdwRemap[i];
        for (int j = 0; j < 3; ++j)
            indices[newFace * 3 + j] = dstIndices[i * 3 + j];
    }

    n_delete_array(dstIndices);
    n_delete_array(pdwRemap);
}

void
nD3D9Mesh::OptimizeVertices(float* vertices, ushort* indices, int numVertices, int numFaces)
{
    DWORD* pdwRemap = n_new_array(DWORD, numVertices);

    D3DXOptimizeVertices(indices, numFaces, numVertices, FALSE, pdwRemap);

    // remap vertices
    float* dstVertices = n_new_array(float, numVertices * this->GetVertexWidth());
    n_assert(dstVertices);
    memcpy(dstVertices, vertices, numVertices * this->GetVertexWidth() * sizeof(float));

    for (int i = 0; i < numVertices; ++i)
    {
        float* src = dstVertices + (i * this->GetVertexWidth());
        float* dst = vertices + (pdwRemap[i] * this->GetVertexWidth());
        memcpy(dst, src, this->GetVertexWidth() * sizeof(float));
    }

    // remap triangles
    for (int faceIndex = 0; faceIndex < numFaces; ++faceIndex)
    {
        for (int index = 0; index < 3; ++index)
        {
            indices[faceIndex * 3 + index] = (ushort) pdwRemap[indices[faceIndex * 3 + index]];
        }
    }

    n_delete_array(dstVertices);
    n_delete_array(pdwRemap);
}

*/

}