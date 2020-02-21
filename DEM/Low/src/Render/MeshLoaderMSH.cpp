#include "MeshLoaderMSH.h"
#include <Render/MeshData.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <Data/RAMData.h>

namespace Resources
{
#pragma pack(push, 1)
struct CMSHMeshGroup
{
	U32     FirstVertex;
	U32     VertexCount;
	U32     FirstIndex;
	U32     IndexCount;
	U8      TopologyCode;
	vector3 AABBMin;
	vector3 AABBMax;
};
#pragma pack(pop)

const Core::CRTTI& CMeshLoaderMSH::GetResultType() const
{
	return Render::CMeshData::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CMeshLoaderMSH::CreateResource(CStrID UID)
{
	if (!pResMgr) return nullptr;

	const char* pOutSubId;
	IO::PStream Stream = pResMgr->CreateResourceStream(UID, pOutSubId, IO::SAP_SEQUENTIAL);
	if (!Stream || !Stream->Open()) return nullptr;

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
		U8 SemanticCode, FormatCode, Index, StreamIndex;
		if (!Reader.Read(SemanticCode)) return nullptr;
		if (!Reader.Read(FormatCode)) return nullptr;
		if (!Reader.Read(Index)) return nullptr;
		if (!Reader.Read(StreamIndex)) return nullptr;

		Component.Format = static_cast<Render::EVertexComponentFormat>(FormatCode);
		Component.Semantic = static_cast<Render::EVertexComponentSemantic>(SemanticCode);
		Component.Index = Index;
		Component.Stream = StreamIndex;
		Component.OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
		Component.UserDefinedName = nullptr;
		Component.PerInstanceData = false;
	}

	std::vector<Render::CPrimitiveGroup> Groups(GroupCount);
	for (auto& MeshGroup : Groups)
	{
		CMSHMeshGroup Group;
		Reader.Read(Group);

		MeshGroup.FirstVertex = Group.FirstVertex;
		MeshGroup.VertexCount = Group.VertexCount;
		MeshGroup.FirstIndex = Group.FirstIndex;
		MeshGroup.IndexCount = Group.IndexCount;
		MeshGroup.Topology = static_cast<Render::EPrimitiveTopology>(Group.TopologyCode);
		MeshGroup.AABB.Min = Group.AABBMin;
		MeshGroup.AABB.Max = Group.AABBMax;
	}

	U32 VertexStartPos, IndexStartPos;
	if (!Reader.Read(VertexStartPos)) return nullptr;
	if (!Reader.Read(IndexStartPos)) return nullptr;

	if (VertexCount)
	{
		//!!!can map data through MMF instead!
		Stream->Seek(VertexStartPos, IO::Seek_Begin);
		const auto DataSize = VertexCount * MeshData->GetVertexSize();
		MeshData->VBData.reset(n_new(Data::CRAMDataMallocAligned(DataSize, 16)));
		Stream->Read(MeshData->VBData->GetPtr(), DataSize);
	}

	if (IndexCount)
	{
		//!!!can map data through MMF instead!
		Stream->Seek(IndexStartPos, IO::Seek_Begin);
		const auto DataSize = IndexCount * IndexSize;
		MeshData->IBData.reset(n_new(Data::CRAMDataMallocAligned(DataSize, 16)));
		Stream->Read(MeshData->IBData->GetPtr(), DataSize);
	}

	MeshData->InitGroups(Groups.data(), Groups.size(), Groups.size(), 1, false, false);

	return MeshData;
}
//---------------------------------------------------------------------

}
