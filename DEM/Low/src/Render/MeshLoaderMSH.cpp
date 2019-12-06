#include "MeshLoaderMSH.h"
#include <Render/MeshData.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <Data/RAMData.h>

namespace Resources
{

const Core::CRTTI& CMeshLoaderMSH::GetResultType() const
{
	return Render::CMeshData::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CMeshLoaderMSH::CreateResource(CStrID UID)
{
	if (!pResMgr) return nullptr;

	const char* pOutSubId;
	IO::PStream Stream = pResMgr->CreateResourceStream(UID, pOutSubId);
	if (!Stream || !Stream->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL)) return nullptr;

	IO::CBinaryReader Reader(*Stream);

	U32 Magic;
	if (!Reader.Read(Magic) || Magic != 'MESH') return nullptr;

	U32 FormatVersion;
	if (!Reader.Read(FormatVersion)) return nullptr;

	U32 GroupCount, VertexCount, IndexCount;
	if (!Reader.Read(GroupCount)) return nullptr;
	if (!Reader.Read(VertexCount)) return nullptr;
	if (!Reader.Read(IndexCount)) return nullptr;

	U8 IndexSize;
	if (!Reader.Read(IndexSize)) return nullptr;

	Render::PMeshData MeshData = n_new(Render::CMeshData);
	MeshData->IndexType = (IndexSize == 4) ? Render::Index_32 : Render::Index_16;
	MeshData->VertexCount = VertexCount;
	MeshData->IndexCount = IndexCount;

	U32 VertexComponentCount;
	if (!Reader.Read(VertexComponentCount)) return nullptr;

	MeshData->VertexFormat.resize(VertexComponentCount);
	for (auto& Component : MeshData->VertexFormat)
	{
		//
	}

	std::vector<Render::CPrimitiveGroup> Groups(GroupCount);
	for (auto& MeshGroup : Groups)
	{
		MeshGroup.FirstVertex = Group.firstVertex;
		MeshGroup.VertexCount = Group.numVertices;
		MeshGroup.FirstIndex = Group.firstTriangle * 3;
		MeshGroup.IndexCount = Group.numTriangles * 3;
		MeshGroup.Topology = Render::Prim_TriList;
	}

	//!!!can map data through MMF instead!
	UPTR DataSize = Header.numVertices * Header.vertexWidth * sizeof(float);
	MeshData->VBData.reset(n_new(Data::CRAMDataMallocAligned(DataSize, 16)));
	Stream->Read(MeshData->VBData->GetPtr(), DataSize);

	//!!!can map data through MMF instead!
	DataSize = Header.numIndices * sizeof(U16);
	MeshData->IBData.reset(n_new(Data::CRAMDataMallocAligned(DataSize, 16)));
	Stream->Read(MeshData->IBData->GetPtr(), DataSize);

	MeshData->InitGroups(Groups.data(), Groups.size(), Groups.size(), 1, false, false);

	return MeshData;
}
//---------------------------------------------------------------------

}
