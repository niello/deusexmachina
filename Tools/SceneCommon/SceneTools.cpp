#include <SceneTools.h>
#include <Logging.h>
#include <Utils.h>
#include <ParamsUtils.h>
#include <meshoptimizer.h>
#include <acl/core/ansi_allocator.h>
#include <acl/core/unique_ptr.h>
#include <acl/algorithm/uniformly_sampled/encoder.h>

namespace acl
{
	typedef std::unique_ptr<AnimationClip, Deleter<AnimationClip>> AnimationClipPtr;
	typedef std::unique_ptr<RigidSkeleton, Deleter<RigidSkeleton>> RigidSkeletonPtr;
}

namespace fs = std::filesystem;

void ProcessGeometry(const std::vector<CVertex>& RawVertices, const std::vector<unsigned int>& RawIndices,
	std::vector<CVertex>& Vertices, std::vector<unsigned int>& Indices)
{
	if (RawIndices.empty())
	{
		// Index not indexed mesh
		Indices.resize(RawVertices.size());
		const auto VertexCount = meshopt_generateVertexRemap(Indices.data(), nullptr, RawVertices.size(), RawVertices.data(), RawVertices.size(), sizeof(CVertex));

		Vertices.resize(VertexCount);
		meshopt_remapVertexBuffer(Vertices.data(), RawVertices.data(), RawVertices.size(), sizeof(CVertex), Indices.data());
	}
	else
	{
		// Remap indexed mesh
		std::vector<unsigned int> Remap(RawVertices.size());
		const auto VertexCount = meshopt_generateVertexRemap(Remap.data(), RawIndices.data(), RawIndices.size(), RawVertices.data(), RawVertices.size(), sizeof(CVertex));

		Vertices.resize(VertexCount);
		meshopt_remapVertexBuffer(Vertices.data(), RawVertices.data(), RawVertices.size(), sizeof(CVertex), Remap.data());

		Indices.resize(RawIndices.size());
		meshopt_remapIndexBuffer(Indices.data(), RawIndices.data(), RawIndices.size(), Remap.data());
	}

	meshopt_optimizeVertexCache(Indices.data(), Indices.data(), Indices.size(), Vertices.size());

	meshopt_optimizeOverdraw(Indices.data(), Indices.data(), Indices.size(), &Vertices[0].Position.x, Vertices.size(), sizeof(CVertex), 1.05f);

	meshopt_optimizeVertexFetch(Vertices.data(), Indices.data(), Indices.size(), Vertices.data(), Vertices.size(), sizeof(CVertex));

	// TODO: meshopt_generateShadowIndexBuffer for Z prepass and shadow rendering.
	// Also can separate positions from all other data into 2 vertex streams, and use only positions for shadows & Z prepass.
}
//---------------------------------------------------------------------

void WriteVertexComponent(std::ostream& Stream, EVertexComponentSemantic Semantic, EVertexComponentFormat Format, uint8_t Index, uint8_t StreamIndex)
{
	WriteStream(Stream, static_cast<uint8_t>(Semantic));
	WriteStream(Stream, static_cast<uint8_t>(Format));
	WriteStream(Stream, Index);
	WriteStream(Stream, StreamIndex);
}
//---------------------------------------------------------------------

bool WriteDEMMesh(const fs::path& DestPath, const std::map<std::string, CMeshData>& SubMeshes, const CVertexFormat& VertexFormat, size_t BoneCount, CThreadSafeLog& Log)
{
	fs::create_directories(DestPath.parent_path());

	std::ofstream File(DestPath, std::ios_base::binary | std::ios_base::trunc);
	if (!File)
	{
		Log.LogError("Error opening an output file " + DestPath.string());
		return false;
	}

	size_t TotalVertices = 0;
	size_t TotalIndices = 0;
	for (const auto& Pair : SubMeshes)
	{
		TotalVertices += Pair.second.Vertices.size();
		TotalIndices += Pair.second.Indices.size();
	}

	WriteStream<uint32_t>(File, 'MESH');     // Format magic value
	WriteStream<uint32_t>(File, 0x00010000); // Version 0.1.0.0

	WriteStream(File, static_cast<uint32_t>(SubMeshes.size()));
	WriteStream(File, static_cast<uint32_t>(TotalVertices));
	WriteStream(File, static_cast<uint32_t>(TotalIndices));

	// One index size in bytes
	const bool Indices32 = (TotalVertices > std::numeric_limits<uint16_t>().max());
	if (Indices32)
	{
		Log.LogWarning("Mesh " + DestPath.filename().string() + " has " + std::to_string(TotalVertices) + " vertices and will use 32-bit indices");
		WriteStream<uint8_t>(File, 4);
	}
	else
	{
		WriteStream<uint8_t>(File, 2);
	}

	const uint32_t VertexComponentCount =
		1 + // Position
		VertexFormat.NormalCount +
		VertexFormat.TangentCount +
		VertexFormat.BitangentCount +
		VertexFormat.UVCount +
		VertexFormat.ColorCount +
		(VertexFormat.BonesPerVertex ? 2 : 0); // Blend indices and weights

	WriteStream(File, VertexComponentCount);

	WriteVertexComponent(File, EVertexComponentSemantic::VCSem_Position, EVertexComponentFormat::VCFmt_Float32_3, 0, 0);

	for (uint8_t i = 0; i < VertexFormat.NormalCount; ++i)
		WriteVertexComponent(File, EVertexComponentSemantic::VCSem_Normal, EVertexComponentFormat::VCFmt_Float32_3, i, 0);

	for (uint8_t i = 0; i < VertexFormat.TangentCount; ++i)
		WriteVertexComponent(File, EVertexComponentSemantic::VCSem_Tangent, EVertexComponentFormat::VCFmt_Float32_3, i, 0);

	for (uint8_t i = 0; i < VertexFormat.BitangentCount; ++i)
		WriteVertexComponent(File, EVertexComponentSemantic::VCSem_Bitangent, EVertexComponentFormat::VCFmt_Float32_3, i, 0);

	for (uint8_t i = 0; i < VertexFormat.ColorCount; ++i)
		WriteVertexComponent(File, EVertexComponentSemantic::VCSem_Color, EVertexComponentFormat::VCFmt_UInt8_4_Norm, i, 0);

	for (uint8_t i = 0; i < VertexFormat.UVCount; ++i)
		WriteVertexComponent(File, EVertexComponentSemantic::VCSem_TexCoord, EVertexComponentFormat::VCFmt_Float32_2, i, 0);

	if (VertexFormat.BonesPerVertex)
	{
		if (BoneCount > 256)
			// Could use unsigned, but it is not supported in D3D9. > 32k bones is nonsence anyway.
			WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneIndices, EVertexComponentFormat::VCFmt_SInt16_4, 0, 0);
		else
			WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneIndices, EVertexComponentFormat::VCFmt_UInt8_4, 0, 0);

		// Max 4 bones per vertex are supported, at least for now
		assert(VertexFormat.BonesPerVertex < 5);

		if (VertexFormat.BonesPerVertex == 1)
			WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneWeights, EVertexComponentFormat::VCFmt_Float32_1, 0, 0);
		else if (VertexFormat.BonesPerVertex == 2)
			WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneWeights, EVertexComponentFormat::VCFmt_Float32_2, 0, 0);
		else if (VertexFormat.BonesPerVertex == 3)
			WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneWeights, EVertexComponentFormat::VCFmt_Float32_3, 0, 0);
		else
			WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneWeights, EVertexComponentFormat::VCFmt_Float32_4, 0, 0);
	}

	// Save mesh groups (always 1 LOD now, may change later)

	// TODO: test if index offset is needed, each submesh starts from zero now
	for (const auto& Pair : SubMeshes)
	{
		WriteStream<uint32_t>(File, 0);                                            // First vertex
		WriteStream(File, static_cast<uint32_t>(Pair.second.Vertices.size()));     // Vertex count
		WriteStream<uint32_t>(File, 0);                                            // First index
		WriteStream(File, static_cast<uint32_t>(Pair.second.Indices.size()));      // Index count
		WriteStream(File, static_cast<uint8_t>(EPrimitiveTopology::Prim_TriList));
		WriteStream(File, Pair.second.AABBMin);
		WriteStream(File, Pair.second.AABBMax);
	}

	// Align vertex and index data offsets to 16 bytes. It should speed up loading from memory-mapped file.
	// TODO: test!

	// Delay writing vertex and index data offsets.
	const auto DataOffsetsPos = File.tellp();
	WriteStream<uint32_t>(File, 0);
	WriteStream<uint32_t>(File, 0);

	uint32_t VertexStartPos = static_cast<uint32_t>(DataOffsetsPos) + 2 * sizeof(uint32_t);
	if (const auto VertexStartPadding = VertexStartPos % 16)
	{
		VertexStartPos += (16 - VertexStartPadding);
		for (auto i = VertexStartPadding; i < 16; ++i)
			WriteStream<uint8_t>(File, 0);
	}

	for (const auto& Pair : SubMeshes)
	{
		const auto& Vertices = Pair.second.Vertices;
		for (const auto& Vertex : Vertices)
		{
			WriteStream(File, Vertex.Position);

			if (VertexFormat.NormalCount) WriteStream(File, Vertex.Normal);
			if (VertexFormat.TangentCount) WriteStream(File, Vertex.Tangent);
			if (VertexFormat.BitangentCount) WriteStream(File, Vertex.Bitangent);
			if (VertexFormat.ColorCount) WriteStream(File, Vertex.Color);

			for (size_t i = 0; i < VertexFormat.UVCount; ++i)
				WriteStream(File, Vertex.UV[i]);

			if (VertexFormat.BonesPerVertex)
			{
				// Blend indices are always 4-component
				for (int i = 0; i < 4; ++i)
				{
					const int BoneIndex = (i < MaxBonesPerVertex) ? Vertex.BlendIndices[i] : 0;
					if (BoneCount > 256)
						WriteStream(File, static_cast<int16_t>(BoneIndex));
					else
						WriteStream(File, static_cast<uint8_t>(BoneIndex));
				}

				File.write(reinterpret_cast<const char*>(Vertex.BlendWeights), VertexFormat.BonesPerVertex * sizeof(float));
			}
		}
	}

	uint32_t IndexStartPos = static_cast<uint32_t>(File.tellp());
	if (const auto IndexStartPadding = IndexStartPos % 16)
	{
		IndexStartPos += (16 - IndexStartPadding);
		for (auto i = IndexStartPadding; i < 16; ++i)
			WriteStream<uint8_t>(File, 0);
	}

	for (const auto& Pair : SubMeshes)
	{
		const auto& Indices = Pair.second.Indices;
		if (Indices32)
		{
			static_assert(sizeof(unsigned int) == 4);
			File.write(reinterpret_cast<const char*>(Indices.data()), Indices.size() * 4);
		}
		else
		{
			for (auto Index : Indices)
				WriteStream(File, static_cast<uint16_t>(Index));
		}
	}

	// Write delayed values
	File.seekp(DataOffsetsPos);
	WriteStream<uint32_t>(File, VertexStartPos);
	WriteStream<uint32_t>(File, IndexStartPos);

	return true;
}
//---------------------------------------------------------------------