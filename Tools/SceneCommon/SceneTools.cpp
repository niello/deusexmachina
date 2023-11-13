#include <SceneTools.h>
#include <Logging.h>
#include <Utils.h>
#include <ParamsUtils.h>
#include <meshoptimizer.h>
#include <acl/core/ansi_allocator.h>
#include <acl/core/unique_ptr.h>
#include <acl/algorithm/uniformly_sampled/encoder.h>
#include <rtm/quatf.h>
#include <IL/il.h>
#include <LinearMath/btConvexHullComputer.h>
#include <regex>

namespace acl
{
	typedef std::unique_ptr<AnimationClip, Deleter<AnimationClip>> AnimationClipPtr;
	typedef std::unique_ptr<RigidSkeleton, Deleter<RigidSkeleton>> RigidSkeletonPtr;
}

namespace fs = std::filesystem;

// TODO: use iluErrorString?
static const char* GetDevILErrorText(ILenum ErrorCode)
{
	switch (ErrorCode)
	{
		case IL_NO_ERROR: return "No error";
		case IL_INVALID_ENUM: return "Invalid enum";
		case IL_OUT_OF_MEMORY: return "Out of memory";
		case IL_FORMAT_NOT_SUPPORTED: return "Format not supported";
		case IL_INTERNAL_ERROR: return "Internal error";
		case IL_INVALID_VALUE: return "Invalid value";
		case IL_ILLEGAL_OPERATION: return "Illegal operation";
		case IL_ILLEGAL_FILE_VALUE: return "Illegal file value";
		case IL_INVALID_FILE_HEADER: return "Invalid file header";
		case IL_INVALID_PARAM: return "Invalid param";
		case IL_COULD_NOT_OPEN_FILE: return "Couldn't open file";
		case IL_INVALID_EXTENSION: return "Invalid extension";
		case IL_FILE_ALREADY_EXISTS: return "File already exists";
		case IL_OUT_FORMAT_SAME: return "Out format same";
		case IL_STACK_OVERFLOW: return "Stack overflow";
		case IL_STACK_UNDERFLOW: return "Stack underflow";
		case IL_INVALID_CONVERSION: return "Invalid conversion";
		case IL_BAD_DIMENSIONS: return "Bad dimensions";
		case IL_FILE_READ_ERROR: return "File read or write error"; // IL_FILE_WRITE_ERROR is the same code
		case IL_LIB_GIF_ERROR: return "GIF error";
		case IL_LIB_JPEG_ERROR: return "JPEG error";
		case IL_LIB_PNG_ERROR: return "PNG error";
		case IL_LIB_TIFF_ERROR: return "TIFF error";
		case IL_LIB_MNG_ERROR: return "MNG error";
		case IL_LIB_JP2_ERROR: return "JP2 error";
		case IL_LIB_EXR_ERROR: return "EXR error";
		case IL_UNKNOWN_ERROR: return "Unknown DevIL error";
		default: return "Unknown DevIL error code";
	}
}
//---------------------------------------------------------------------

std::string GetRelativeNodePath(std::vector<std::string>&& From, std::vector<std::string>&& To)
{
	std::string RelPath;

	while (!From.empty() &&
		!To.empty() &&
		From.back() == To.back())
	{
		From.pop_back();
		To.pop_back();
	}

	for (const auto& NodeID : From)
	{
		if (!RelPath.empty()) RelPath += '.';
		RelPath += '^';
	}

	if (!To.empty()) To.erase(To.begin());
	for (auto It = To.crbegin(); It != To.crend(); ++It)
	{
		if (!RelPath.empty()) RelPath += '.';
		RelPath += GetValidNodeName(*It);
	}

	return RelPath;
}
//---------------------------------------------------------------------

bool LoadSceneSettings(const std::filesystem::path& Path, CSceneSettings& Out)
{
	//!!!FIXME: don't convert path to string! Use path in API!
	Data::CParams SceneSettings;
	if (!ParamsUtils::LoadParamsFromHRD(Path.generic_string().c_str(), SceneSettings))
	{
		std::cout << "Couldn't load scene settings from " << Path;
		return false;
	}

	const Data::CParams* pMap;
	if (ParamsUtils::TryGetParam(pMap, SceneSettings, "Effects"))
		for (const auto& Pair : *pMap)
			Out.EffectsByType.emplace(Pair.first.ToString(), Pair.second.GetValue<std::string>());

	if (ParamsUtils::TryGetParam(pMap, SceneSettings, "Params"))
		for (const auto& Pair : *pMap)
			Out.EffectParamAliases.emplace(Pair.first.ToString(), Pair.second.GetValue<std::string>());

	return true;
}
//---------------------------------------------------------------------

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

bool WriteDEMMesh(const fs::path& DestPath, const std::map<std::string, CMeshGroup>& SubMeshes, const CVertexFormat& VertexFormat, size_t BoneCount, CThreadSafeLog& Log)
{
	fs::create_directories(DestPath.parent_path());

	std::ofstream File(DestPath, std::ios_base::binary | std::ios_base::trunc);
	if (!File)
	{
		Log.LogError("Error opening an output file " + DestPath.string());
		return false;
	}

	if (VertexFormat.BlendWeightSize != 8 && VertexFormat.BlendWeightSize != 16 && VertexFormat.BlendWeightSize != 32)
		Log.LogWarning("Unsupported blend weight size, defaulting to full-precision floats (32). Supported values are 8/16/32.");

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
			// Could use unsigned, but it is not supported in D3D9. > 32k bones is nonsense anyway.
			WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneIndices, EVertexComponentFormat::VCFmt_SInt16_4, 0, 0);
		else
			WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneIndices, EVertexComponentFormat::VCFmt_UInt8_4, 0, 0);

		// Max 4 bones per vertex are supported, at least for now
		assert(VertexFormat.BonesPerVertex < 5);

		if (VertexFormat.BlendWeightSize == 8)
		{
			WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneWeights, EVertexComponentFormat::VCFmt_UInt8_4_Norm, 0, 0);
		}
		else if (VertexFormat.BlendWeightSize == 16)
		{
			// FIXME: shader defaults missing components to (0, 0, 0, 1), making the last weight incorrect
			//if (VertexFormat.BonesPerVertex <= 2)
			//	WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneWeights, EVertexComponentFormat::VCFmt_UInt16_2_Norm, 0, 0);
			//else
				WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneWeights, EVertexComponentFormat::VCFmt_UInt16_4_Norm, 0, 0);
		}
		else
		{
			// FIXME: shader defaults missing components to (0, 0, 0, 1), making the last weight incorrect
			//if (VertexFormat.BonesPerVertex == 1)
			//	WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneWeights, EVertexComponentFormat::VCFmt_Float32_1, 0, 0);
			//else if (VertexFormat.BonesPerVertex == 2)
			//	WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneWeights, EVertexComponentFormat::VCFmt_Float32_2, 0, 0);
			//else if (VertexFormat.BonesPerVertex == 3)
			//	WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneWeights, EVertexComponentFormat::VCFmt_Float32_3, 0, 0);
			//else
				WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneWeights, EVertexComponentFormat::VCFmt_Float32_4, 0, 0);
		}
	}

	// Save mesh groups (always 1 LOD now, may change later)

	// TODO: test if index offset is needed, each submesh starts from zero now
	for (const auto& [SubMeshID, SubMesh] : SubMeshes)
	{
		WriteStream<uint32_t>(File, 0);                                            // First vertex
		WriteStream(File, static_cast<uint32_t>(SubMesh.Vertices.size()));         // Vertex count
		WriteStream<uint32_t>(File, 0);                                            // First index
		WriteStream(File, static_cast<uint32_t>(SubMesh.Indices.size()));          // Index count
		WriteStream(File, static_cast<uint8_t>(EPrimitiveTopology::Prim_TriList));
		WriteStream(File, SubMesh.AABB.Min);
		WriteStream(File, SubMesh.AABB.Max);
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

	assert(!(static_cast<uint32_t>(File.tellp()) % 16));

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

				if (VertexFormat.BlendWeightSize == 8)
				{
					WriteStream<uint32_t>(File, Vertex.BlendWeights8);
				}
				else if (VertexFormat.BlendWeightSize == 16)
				{
					// FIXME: shader defaults missing components to (0, 0, 0, 1), making the last weight incorrect
					//if (VertexFormat.BonesPerVertex <= 2)
					//	File.write(reinterpret_cast<const char*>(Vertex.BlendWeights16), 2 * sizeof(uint16_t));
					//else
						File.write(reinterpret_cast<const char*>(Vertex.BlendWeights16), 4 * sizeof(uint16_t));
				}
				else
				{
					// FIXME: shader defaults missing components to (0, 0, 0, 1), making the last weight incorrect
					//File.write(reinterpret_cast<const char*>(Vertex.BlendWeights32), VertexFormat.BonesPerVertex * sizeof(float));
					File.write(reinterpret_cast<const char*>(Vertex.BlendWeights32), 4 * sizeof(float));
				}
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

	assert(!(static_cast<uint32_t>(File.tellp()) % 16));

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

	Log.LogInfo(DestPath.filename().generic_string() + " " + std::to_string(File.tellp()) + " bytes saved");

	// Write delayed values
	File.seekp(DataOffsetsPos);
	WriteStream<uint32_t>(File, VertexStartPos);
	WriteStream<uint32_t>(File, IndexStartPos);

	return true;
}
//---------------------------------------------------------------------

bool ReadDEMMeshVertexPositions(const std::filesystem::path& Path, std::vector<float3>& Out, CThreadSafeLog& Log)
{
	std::ifstream File(Path, std::ios_base::binary);
	if (!File)
	{
		Log.LogError("Can't open mesh file " + Path.generic_string());
		return false;
	}

	// Check format magic and version
	if (ReadStream<uint32_t>(File) != 'MESH') return false;
	if (ReadStream<uint32_t>(File) != 0x00010000) return false;

	const auto SubMeshCount = ReadStream<uint32_t>(File);
	const auto VertexCount = ReadStream<uint32_t>(File);

	Out.resize(VertexCount);
	if (!VertexCount) return true;

	const auto IndexCount = ReadStream<uint32_t>(File);

	ReadStream<uint8_t>(File); // Skip index size

	size_t PosOffset = 0;
	size_t VertexSize = 0;
	bool PosFound = false;
	const auto VertexComponentCount = ReadStream<uint32_t>(File);
	for (uint32_t i = 0; i < VertexComponentCount; ++i)
	{
		const auto Semantic = static_cast<EVertexComponentSemantic>(ReadStream<uint8_t>(File));
		const auto Format = static_cast<EVertexComponentFormat>(ReadStream<uint8_t>(File));
		const size_t ComponentSize = GetVertexComponentSize(Format);

		VertexSize += ComponentSize;

		if (Semantic == EVertexComponentSemantic::VCSem_Position)
			PosFound = true;
		else if (!PosFound)
			PosOffset += ComponentSize;

		// Skip Index and StreamIndex
		ReadStream<uint8_t>(File);
		ReadStream<uint8_t>(File);
	}

	// Skip mesh group definitions, 41 byte each
	File.seekg(SubMeshCount * 41, std::ios_base::_Seekcur);

	const auto VertexDataOffset = ReadStream<uint32_t>(File);
	const auto IndexDataOffset = ReadStream<uint32_t>(File);

	// Seek to the first vertex position
	File.seekg(VertexDataOffset + PosOffset, std::ios_base::_Seekbeg);
	const auto RemainingVertexSize = VertexSize - sizeof(float3);

	for (uint32_t i = 0; i < VertexCount; ++i)
	{
		ReadStream(File, Out[i]);
		if (i < VertexCount - 1 && RemainingVertexSize > 0)
			File.seekg(RemainingVertexSize, std::ios_base::_Seekcur);
	}

	return true;
}
//---------------------------------------------------------------------

bool WriteDEMSkin(const fs::path& DestPath, const std::vector<CBone>& Bones, CThreadSafeLog& Log)
{
	fs::create_directories(DestPath.parent_path());

	std::ofstream File(DestPath, std::ios_base::binary | std::ios_base::trunc);
	if (!File)
	{
		Log.LogError("Error opening an output file " + DestPath.string());
		return false;
	}

	WriteStream<uint32_t>(File, 'SKIN');        // Format magic value
	WriteStream<uint32_t>(File, 0x00010000);    // Version 0.1.0.0
	WriteStream(File, static_cast<uint32_t>(Bones.size()));
	WriteStream<uint32_t>(File, 0);             // Padding to align matrices offset to 16 bytes boundary

	assert(!(static_cast<uint32_t>(File.tellp()) % 16));

	for (const auto& Bone : Bones)
		WriteStream(File, Bone.InvLocalBindPose);

	for (const auto& Bone : Bones)
	{
		WriteStream(File, Bone.ParentBoneIndex);
		WriteStream(File, Bone.ID);
	}

	Log.LogInfo(DestPath.filename().generic_string() + " " + std::to_string(File.tellp()) + " bytes saved");

	return true;
}
//---------------------------------------------------------------------

bool WriteDEMAnimation(const std::filesystem::path& DestPath, acl::IAllocator& ACLAllocator,
	const acl::AnimationClip& Clip, const std::vector<std::string>& NodeNames,
	const CLocomotionInfo* pLocomotionInfo, CThreadSafeLog& Log)
{
	const auto AnimName = DestPath.filename().string();
	const auto& Skeleton = Clip.get_skeleton();
	const auto NodeCount = Skeleton.get_num_bones();

	if (!NodeCount)
	{
		Log.LogWarning(std::string("Skipped saving empty animation ") + AnimName);
		return true;
	}

	if (Skeleton.get_bone(0).parent_index != acl::k_invalid_bone_index)
	{
		Log.LogError("Animation " + AnimName + " doesn't start from the root node (may be broken)!");
		return false;
	}

	acl::TransformErrorMetric ACLErrorMetric;
	acl::CompressionSettings ACLSettings = acl::get_default_compression_settings();
	ACLSettings.error_metric = &ACLErrorMetric;

	acl::OutputStats Stats;
	acl::CompressedClip* CompressedClip = nullptr;
	acl::ErrorResult ErrorResult = acl::uniformly_sampled::compress_clip(ACLAllocator, Clip, ACLSettings, CompressedClip, Stats);
	if (!ErrorResult.empty())
	{
		Log.LogWarning(std::string("ACL failed to compress animation ") + AnimName + " for one of skeletons");
		return true;
	}

	Log.LogDebug(std::string("ACL compressed animation ") + AnimName + " to " + std::to_string(CompressedClip->get_size()) + " bytes");

	fs::create_directories(DestPath.parent_path());

	std::ofstream File(DestPath, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
	if (!File)
	{
		Log.LogError("Error opening an output file " + DestPath.string());
		return false;
	}

	WriteStream<uint32_t>(File, 'ANIM');     // Format magic value
	WriteStream<uint32_t>(File, 0x00010000); // Version 0.1.0.0

	WriteStream<float>(File, Clip.get_duration());
	WriteStream<uint32_t>(File, Clip.get_num_samples());

	WriteStream<uint16_t>(File, NodeCount);

	// Write skeleton info to be able to bind animation to the node hierarchy in DEM
	// Note that DEM requires all nodes without a parent to have a full path relative
	// to the root in their IDs. So for the root itself ID must be empty.
	WriteStream<uint16_t>(File, Skeleton.get_bone(0).parent_index);
	WriteStream(File, std::string{});
	for (uint16_t i = 1; i < NodeCount; ++i)
	{
		WriteStream<uint16_t>(File, Skeleton.get_bone(i).parent_index);
		WriteStream(File, NodeNames[i]);
	}

	// For locomotion clips save locomotion info
	WriteStream<uint8_t>(File, pLocomotionInfo != nullptr);
	if (pLocomotionInfo)
	{
		const float Speed = CompareFloat(pLocomotionInfo->SpeedFromFeet, 0.f) ? pLocomotionInfo->SpeedFromRoot : pLocomotionInfo->SpeedFromFeet;
		WriteStream<float>(File, Speed);
		WriteStream(File, pLocomotionInfo->CycleStartFrame);
		WriteStream(File, pLocomotionInfo->LeftFootOnGroundFrame);
		WriteStream(File, pLocomotionInfo->RightFootOnGroundFrame);
		WriteVectorToStream(File, pLocomotionInfo->Phases);
		WriteMapToStream(File, pLocomotionInfo->PhaseNormalizedTimes);
	}

	// Align compressed clip data in a file to 16 bytes (plan to use memory mapped files for faster loading)
	uint32_t CurrPos = static_cast<uint32_t>(File.tellp()) + sizeof(uint8_t); // Added padding size
	if (const auto PaddingStart = CurrPos % 16)
	{
		WriteStream<uint8_t>(File, 16 - PaddingStart);
		for (auto i = PaddingStart; i < 16; ++i)
			WriteStream<uint8_t>(File, 0);
	}
	else
	{
		WriteStream<uint8_t>(File, 0);
	}

	assert(!(static_cast<uint32_t>(File.tellp()) % 16));

	File.write(reinterpret_cast<const char*>(CompressedClip), CompressedClip->get_size());

	Log.LogInfo(DestPath.filename().generic_string() + " " + std::to_string(File.tellp()) + " bytes saved" + (pLocomotionInfo ? ", including locomotion info" : ""));

	return true;
}
//---------------------------------------------------------------------

// FIXME: to common ancestor of FBX & glTF tools, to have access to a CF tool class data
// Name = Task.TaskID.ToString()
// TaskParams = Task.Params
// DestDir = GetPath(Task.Params, "Output");
bool WriteDEMScene(const std::filesystem::path& DestDir, const std::string& Name, Data::CParams&& Nodes,
	const Data::CSchemeSet& Schemes, const Data::CParams& TaskParams, bool HRD, bool Binary, bool CreateRoot, CThreadSafeLog& Log)
{
	const std::string TaskName = GetValidResourceName(Name);

	Data::CParams Result;

	const Data::CParams* pTfmParams;
	if (ParamsUtils::TryGetParam(pTfmParams, TaskParams, "Transform"))
	{
		if (!CreateRoot)
		{
			Log.LogError(TaskName + " contains Transform but CreateRoot is false, can't save a scene");
			return false;
		}

		float3 Vec3Value;
		float4 Vec4Value;
		if (ParamsUtils::TryGetParam(Vec3Value, *pTfmParams, "S"))
			Result.emplace_back(CStrID("Scale"), Vec3Value);
		if (ParamsUtils::TryGetParam(Vec4Value, *pTfmParams, "R"))
			Result.emplace_back(CStrID("Rotation"), Vec4Value);
		if (ParamsUtils::TryGetParam(Vec3Value, *pTfmParams, "T"))
			Result.emplace_back(CStrID("Translation"), Vec3Value);
	}

	if (Nodes.size() == 1 && !CreateRoot)
	{
		Result = std::move(Nodes[0].second.GetValue<Data::CParams>());
	}
	else
	{
		if (!CreateRoot)
		{
			Log.LogError(TaskName + " node count is not exactly 1 but CreateRoot is false, can't save a scene");
			return false;
		}

		if (!Nodes.empty())
		{
			Result.emplace_back(CStrID("Children"), std::move(Nodes));
		}
	}

	if (HRD)
	{
		const auto DestPath = DestDir / (TaskName + ".hrd");
		if (!ParamsUtils::SaveParamsToHRD(DestPath.string().c_str(), Result))
		{
			Log.LogError("Error serializing " + TaskName + " to text");
			return false;
		}
	}

	if (Binary)
	{
		const auto DestPath = DestDir / (TaskName + ".scn");
		if (!ParamsUtils::SaveParamsByScheme(DestPath.string().c_str(), Result, CStrID("SceneNode"), Schemes))
		{
			Log.LogError("Error serializing " + TaskName + " to binary");
			return false;
		}
	}

	return true;
}
//---------------------------------------------------------------------

void InitImageProcessing()
{
	ilInit();
}
//---------------------------------------------------------------------

void TermImageProcessing()
{
	ilShutDown();
}
//---------------------------------------------------------------------

static bool IsImageFormatSavingSupported(std::string_view Format)
{
	return Format == "DXT5" || Format == "DXT5nm" || Format == "DDS" || Format == "TGA";
}
//---------------------------------------------------------------------

ILuint LoadILImage(const std::filesystem::path& SrcPath, CThreadSafeLog& Log)
{
	ILuint ImgId = ilGenImage();
	ilBindImage(ImgId);

	if (!ilLoadImage(SrcPath.string().c_str()))
	{
		ilDeleteImage(ImgId);
		Log.LogError("Can't load image " + SrcPath.generic_string() + ", error: " + std::string(GetDevILErrorText(ilGetError())));
		return 0;
	}

	return ImgId;
}
//---------------------------------------------------------------------

void UnloadILImage(ILuint ID)
{
	if (ID) ilDeleteImage(ID);
}
//---------------------------------------------------------------------

CRect GetILImageRect(ILuint ID)
{
	ilBindImage(ID);
	return GetCurrentILImageRect();
}
//---------------------------------------------------------------------

CRect GetCurrentILImageRect()
{
	return CRect(0, 0, ilGetInteger(IL_IMAGE_WIDTH) - 1, ilGetInteger(IL_IMAGE_HEIGHT) - 1);
}
//---------------------------------------------------------------------

bool SaveCurrentILImage(const std::string& DestFormat, std::filesystem::path DestPath, CThreadSafeLog& Log)
{
	fs::create_directories(DestPath.parent_path());

	ilEnable(IL_FILE_OVERWRITE);

	ILboolean Result = IL_FALSE;
	std::string Error;

	if (DestFormat.empty())
	{
		// Save in a source format. Enable some format-specific settings.
		ilSetInteger(IL_TGA_RLE, IL_TRUE);
		Result = ilSaveImage(DestPath.string().c_str());
		Error = std::string(GetDevILErrorText(ilGetError()));
	}
	else if (DestFormat == "DXT5")
	{
		DestPath.replace_extension("dds");
		ilSetInteger(IL_DXTC_FORMAT, IL_DXT5);
		Result = ilSave(IL_DDS, DestPath.string().c_str());
		Error = std::string(GetDevILErrorText(ilGetError()));
	}
	else if (DestFormat == "DXT5nm")
	{
		// FIXME: no normal map compression for now. Use NV texture tools or the like?
		DestPath.replace_extension("dds");
		Result = ilSave(IL_DDS, DestPath.string().c_str());
		Error = std::string(GetDevILErrorText(ilGetError()));

		Log.LogWarning("Save as DDS (no DXT5nm support yet): " + DestPath.generic_string());
	}
	else if (DestFormat == "DDS")
	{
		DestPath.replace_extension("dds");
		Result = ilSave(IL_DDS, DestPath.string().c_str());
		Error = std::string(GetDevILErrorText(ilGetError()));
	}
	else if (DestFormat == "TGA")
	{
		DestPath.replace_extension("tga");
		ilSetInteger(IL_TGA_RLE, IL_TRUE);
		Result = ilSave(IL_TGA, DestPath.string().c_str());
		Error = std::string(GetDevILErrorText(ilGetError()));
	}
	else
	{
		Result = IL_FALSE;
		Error = "Unknown format";
	}

	if (Result)
		Log.LogDebug("Saved" + (DestFormat.empty() ? " in source format" : " as " + DestFormat) + ": " + DestPath.generic_string());
	else
		Log.LogError("Error saving " + DestPath.generic_string() + (DestFormat.empty() ? " in source format" : " as " + DestFormat) + ": " + Error);

	return !!Result;
}
//---------------------------------------------------------------------

bool SaveILImageRegion(ILuint ID, const std::string& DestFormat, std::filesystem::path DestPath, CRect& Region, CThreadSafeLog& Log)
{
	ilBindImage(ID);

	const auto ImageRect = GetCurrentILImageRect();
	Region = Region.Intersection(ImageRect);

	// If dest format is block-compressed, round right & bottom
	if (DestFormat == "DXT5")
	{
		Region.Right += DivCeil(Region.Width(), 4) * 4 - Region.Width();
		Region.Bottom += DivCeil(Region.Height(), 4) * 4 - Region.Height();
	}

	ILuint BlitImgId = 0;
	const bool NeedBlit = (Region.Intersection(ImageRect) != ImageRect);
	if (NeedBlit)
	{
		const auto Channels = ilGetInteger(IL_IMAGE_CHANNELS);
		const auto Format = ilGetInteger(IL_IMAGE_FORMAT);
		const auto Type = ilGetInteger(IL_IMAGE_TYPE);

		BlitImgId = ilGenImage();
		ilBindImage(BlitImgId);
		ilTexImage(Region.Width(), Region.Height(), 1, Channels, Format, Type, nullptr);
		ilDisable(IL_BLIT_BLEND);
		ilBlit(ID, 0, 0, 0, Region.Left, Region.Top, 0, Region.Width(), Region.Height(), 1);
	}

	const bool Result = SaveCurrentILImage(DestFormat, DestPath, Log);

	if (NeedBlit)
	{
		ilDeleteImage(BlitImgId);
		ilBindImage(ID);
	}

	return Result;
}
//---------------------------------------------------------------------

std::string GetTextureDestFormat(const std::filesystem::path& SrcPath, const Data::CParams& TaskParams)
{
	// Search in order for the first matching conversion rule
	// TODO: preload and store as structs?
	std::string DestFormat;
	const Data::CDataArray* pTextures = nullptr;
	if (ParamsUtils::TryGetParam(pTextures, TaskParams, "Textures"))
	{
		for (const auto& Data : *pTextures)
		{
			const std::regex Rule(ParamsUtils::GetParam(Data.GetValue<Data::CParams>(), "Name", std::string{}));
			if (std::regex_match(SrcPath.generic_string(), Rule))
			{
				DestFormat = ParamsUtils::GetParam(Data.GetValue<Data::CParams>(), "DestFormat", std::string{});
				break;
			}
		}
	}

	return DestFormat;
}
//---------------------------------------------------------------------

std::string WriteTexture(const fs::path& SrcPath, const fs::path& DestDir, const std::string& DestFormat, CThreadSafeLog& Log)
{
	const auto RsrcName = GetValidResourceName(SrcPath.stem().string());
	const auto SrcExtension = SrcPath.extension().generic_string();

	fs::path DestPath = DestDir / (RsrcName + SrcExtension);

	// Check if conversion is requested and possible
	if (DestFormat.empty() || !IsImageFormatSavingSupported(DestFormat))
	{
		if (!DestFormat.empty())
			Log.LogWarning("Unknown format '" + DestFormat + "' for " + DestPath.generic_string());

		if (!CopyFile(SrcPath, DestPath, &Log)) return {};
	}
	else
	{
		ILuint ImgId = LoadILImage(SrcPath, Log);
		if (!ImgId) return {};

		const bool Result = SaveCurrentILImage(DestFormat, DestPath, Log);

		ilDeleteImages(1, &ImgId);

		if (!Result) return {};
	}

	return DestPath.generic_string();
}
//---------------------------------------------------------------------

// TODO: check if already created on this launch, don't recreate
std::optional<std::string> GenerateCollisionShape(std::string ShapeType, const std::filesystem::path& ShapeDir,
	const std::string& MeshRsrcName, const CMeshAttrInfo& MeshInfo, const rtm::qvvf& GlobalTfm, const std::map<std::string, std::filesystem::path>& PathAliases, CThreadSafeLog& Log)
{
	trim(ShapeType, " \t\n\r");
	ToLower(ShapeType);
	if (ShapeType.empty()) return std::string{};

	Log.LogInfo(std::string("Mesh ") + MeshRsrcName + " has an autogenerated collision shape: " + ShapeType);

	const float3 Center(
		(MeshInfo.AABB.Max.x + MeshInfo.AABB.Min.x) * 0.5f,
		(MeshInfo.AABB.Max.y + MeshInfo.AABB.Min.y) * 0.5f,
		(MeshInfo.AABB.Max.z + MeshInfo.AABB.Min.z) * 0.5f);
	const float3 Size(
		MeshInfo.AABB.Max.x - MeshInfo.AABB.Min.x,
		MeshInfo.AABB.Max.y - MeshInfo.AABB.Min.y,
		MeshInfo.AABB.Max.z - MeshInfo.AABB.Min.z);
	const float3 Scaling(
		rtm::vector_get_x(GlobalTfm.scale),
		rtm::vector_get_y(GlobalTfm.scale),
		rtm::vector_get_z(GlobalTfm.scale));

	Data::CParams CollisionShape;
	if (ShapeType == "box")
	{
		//!!!NB: AABB only, to use OBB add collision mesh manually!
		CollisionShape.emplace_back(CStrID("Type"), CStrID("Box"));
		CollisionShape.emplace_back(CStrID("Size"), Size);
	}
	else if (ShapeType == "sphere")
	{
		// FIXME: now circumscribed around AABB, need parameter for user to choose?
		CollisionShape.emplace_back(CStrID("Type"), CStrID("Sphere"));
		CollisionShape.emplace_back(CStrID("Radius"), 0.707107f * std::max({ Size.x, Size.y, Size.z }));
	}
	else if (ShapeType == "capsule")
	{
		std::vector<float> Dims{ Size.x, Size.y, Size.z };
		auto MaxIt = std::max_element(Dims.begin(), Dims.end());
		const float MaxDim = *MaxIt;
		const auto Axis = std::distance(Dims.begin(), MaxIt);
		Dims.erase(MaxIt);
		const float Diameter = *std::max_element(Dims.begin(), Dims.end());

		// FIXME: now inscribed into AABB, need parameter for user to choose?
		switch (Axis)
		{
			case 0: CollisionShape.emplace_back(CStrID("Type"), CStrID("CapsuleX")); break;
			case 1: CollisionShape.emplace_back(CStrID("Type"), CStrID("CapsuleY")); break;
			case 2: CollisionShape.emplace_back(CStrID("Type"), CStrID("CapsuleZ")); break;
		}
		CollisionShape.emplace_back(CStrID("Radius"), Diameter * 0.5f);
		CollisionShape.emplace_back(CStrID("Height"), MaxDim - Diameter);
	}
	else if (ShapeType == "convex")
	{
		std::vector<float3> Vertices;
		if (!ReadDEMMeshVertexPositions(ResolvePathAliases(MeshInfo.MeshID, PathAliases), Vertices, Log)) return std::nullopt;

		if (Vertices.empty()) return std::string{};

		btConvexHullComputer Conv;
		Conv.compute(Vertices[0].v, sizeof(float3), Vertices.size(), 0.f, 0.f);

		const int HullVertexCount = Conv.vertices.size();
		if (!HullVertexCount) return std::string{};

		CollisionShape.emplace_back(CStrID("Type"), CStrID("ConvexHull"));

		Data::CDataArray HullVertices(HullVertexCount);
		for (int i = 0; i < HullVertexCount; ++i)
		{
			const auto& HullVertex = Conv.vertices[i];
			HullVertices[i].SetValue(float3(HullVertex.getX(), HullVertex.getY(), HullVertex.getZ()));
		}

		CollisionShape.emplace_back(CStrID("Vertices"), std::move(HullVertices));
	}
	else if (ShapeType == "mesh")
	{
		// BVH mesh?
	}
	else
	{
		//???warning?
		Log.LogError(std::string("Mesh ") + MeshRsrcName + ": can't generate collision shape");
		return std::nullopt;
	}

	if (ShapeType != "convex" && ShapeType != "mesh")
		CollisionShape.emplace_back(CStrID("Offset"), Center);

	CollisionShape.emplace_back(CStrID("Scaling"), Scaling);

	const auto ShapePath = ShapeDir / (MeshRsrcName + ".hrd");
	if (!ParamsUtils::SaveParamsToHRD(ShapePath.string().c_str(), CollisionShape))
	{
		Log.LogError("Error saving colision shape to " + ShapePath.generic_string());
		return std::nullopt;
	}

	return ShapePath.generic_string();
}
//---------------------------------------------------------------------

void FillNodeTransform(const rtm::qvvf& Tfm, Data::CParams& NodeSection)
{
	static const CStrID sidTranslation("Translation");
	static const CStrID sidRotation("Rotation");
	static const CStrID sidScale("Scale");

	constexpr rtm::vector4f Unit3 = { 1.f, 1.f, 1.f, 0.f };
	constexpr rtm::vector4f Zero3 = { 0.f, 0.f, 0.f, 0.f };
	constexpr rtm::quatf IdentityQuat = { 0.f, 0.f, 0.f, 1.f };

	if (!rtm::vector_all_near_equal3(Tfm.scale, Unit3))
		NodeSection.emplace_back(sidScale, float3({ rtm::vector_get_x(Tfm.scale), rtm::vector_get_y(Tfm.scale), rtm::vector_get_z(Tfm.scale) }));

	if (!rtm::quat_near_equal(Tfm.rotation, IdentityQuat))
		NodeSection.emplace_back(sidRotation, float4({ rtm::quat_get_x(Tfm.rotation), rtm::quat_get_y(Tfm.rotation), rtm::quat_get_z(Tfm.rotation), rtm::quat_get_w(Tfm.rotation) }));

	if (!rtm::vector_all_near_equal3(Tfm.translation, Zero3))
		NodeSection.emplace_back(sidTranslation, float3({ rtm::vector_get_x(Tfm.translation), rtm::vector_get_y(Tfm.translation), rtm::vector_get_z(Tfm.translation) }));
}
//---------------------------------------------------------------------

static std::pair<size_t, size_t> FindFootOnGroundFrames(rtm::vector4f UpDir, const std::vector<rtm::vector4f>& FootPositions)
{
	if (FootPositions.empty()) return { std::numeric_limits<size_t>().max(), std::numeric_limits<size_t>().max() };

	std::vector<float> Heights(FootPositions.size());
	for (size_t i = 0; i < FootPositions.size(); ++i)
		Heights[i] = rtm::vector_dot3(FootPositions[i], UpDir);

	const auto MinMax = std::minmax_element(Heights.cbegin(), Heights.cend());
	const float Min = *MinMax.first;
	const float Max = *MinMax.second;
	const float Tolerance = (Max - Min) * 0.001f;

	size_t Start = 0, End = 0;

	// TODO: handle multiple ranges!
	bool PrevFrameDown = false;
	bool CurrFrameDown = (Heights[0] - Min < Tolerance);
	for (size_t i = 1; i < Heights.size(); ++i)
	{
		PrevFrameDown = CurrFrameDown;
		CurrFrameDown = (Heights[i] - Min < Tolerance);
		if (PrevFrameDown == CurrFrameDown) continue;
		if (CurrFrameDown) Start = i;
		else End = (i > 0) ? (i - 1) : (Heights.size() - 1);
	}

	return { Start, End };
}
//---------------------------------------------------------------------

bool ComputeLocomotion(CLocomotionInfo& Out, float FrameRate,
	rtm::vector4f ForwardDir, rtm::vector4f UpDir, rtm::vector4f SideDir,
	const std::vector<rtm::vector4f>& RootPositions,
	const std::vector<rtm::vector4f>& LeftFootPositions,
	const std::vector<rtm::vector4f>& RightFootPositions)
{
	if (LeftFootPositions.empty() || RightFootPositions.empty() || LeftFootPositions.size() != RightFootPositions.size()) return false;

	ForwardDir = rtm::vector_normalize3(ForwardDir);
	UpDir = rtm::vector_normalize3(UpDir);
	SideDir = rtm::vector_normalize3(SideDir);

	const size_t FrameCount = LeftFootPositions.size();

	// Foot phase matching inspired by the method described in:
	// https://cdn.gearsofwar.com/thecoalition/publications/SIGGRAPH%202017%20-%20High%20Performance%20Animation%20in%20Gears%20ofWar%204%20-%20Abstract.pdf

	Out.Phases.resize(FrameCount);
	size_t PhaseStart = 0;

	for (size_t i = 0; i < FrameCount; ++i)
	{
		// Project foot offset onto the locomotion plane (fwd, up) and normalize it to get phase direction
		const auto Offset = rtm::vector_sub(LeftFootPositions[i], RightFootPositions[i]);
		const auto ProjectedOffset = rtm::vector_sub(Offset, rtm::vector_mul(SideDir, rtm::vector_dot3(Offset, SideDir)));
		const auto PhaseDir = rtm::vector_normalize3(ProjectedOffset);

		const float CosA = rtm::vector_dot3(PhaseDir, ForwardDir);
		const float SinA = rtm::vector_dot3(rtm::vector_cross3(PhaseDir, ForwardDir), SideDir);
		const float Angle = std::copysignf(RadToDeg(std::acosf(CosA)), SinA); // Could also use Angle = RadToDeg(std::atan2f(SinA, CosA));

		// Calculate phase in degrees, where:
		// 0 - left behind right
		// 90 - left above right
		// 180 - left in front of right
		// 270 - left below right
		Out.Phases[i] = 180.f - Angle; // map 180 -> -180 to 0 -> 360

		// Find a loop start (a frame where 360 becomes 0)
		// FIXME: is the current heuristic robust enough?
		if (i > 0 && (Out.Phases[i - 1] - Out.Phases[i]) > 180.f) PhaseStart = i;
	}

	Out.CycleStartFrame = static_cast<uint32_t>(PhaseStart);

	// Fill phase to time mapping
	const float InvFrame = (FrameCount > 1) ? (1.f / static_cast<float>(FrameCount - 1)) : 0.f;
	const size_t PhaseEnd = (PhaseStart > 0) ? (PhaseStart - 1) : (FrameCount > 1) ? (FrameCount - 2) : 0;
	size_t FrameIdx = PhaseStart;
	float PrevValue = Out.Phases[FrameIdx];
	Out.PhaseNormalizedTimes.emplace(PrevValue, FrameIdx * InvFrame);
	while (true)
	{
		++FrameIdx;

		// Filter out the last frame due to looping (the last frame is the first frame)
		if (FrameIdx >= FrameCount - 1) FrameIdx = 0;
		if (FrameIdx == PhaseStart) break;

		// Filter out decreasing frames to keep it monotone
		if (Out.Phases[FrameIdx] >= PrevValue)
		{
			PrevValue = Out.Phases[FrameIdx];
			Out.PhaseNormalizedTimes.emplace(PrevValue, FrameIdx * InvFrame);
		}
	};

	// Add sentinel frames to PhaseTimes to simplify runtime search
	Out.PhaseNormalizedTimes.emplace(Out.Phases[PhaseStart] + 360.f, PhaseStart * InvFrame);
	Out.PhaseNormalizedTimes.emplace(Out.Phases[PhaseEnd] - 360.f, PhaseEnd * InvFrame);

	// Locomotion speed is a speed with which a root moves while a foot stands on the ground.
	// Here we detect frame ranges with either foot planted. It is also used for "foot down" events.

	rtm::vector4f RootDiff = { 0.f, 0.f, 0.f, 0.f };
	size_t FramesOnGround = 0;

	// Accumulate motion during the left foot on the ground...
	const auto LeftFoGFrames = FindFootOnGroundFrames(UpDir, LeftFootPositions);
	if (LeftFoGFrames.first < FrameCount)
	{
		Out.LeftFootOnGroundFrame = static_cast<uint32_t>(LeftFoGFrames.first);

		const auto RelRootStart = rtm::vector_sub(RootPositions[LeftFoGFrames.first], LeftFootPositions[LeftFoGFrames.first]);
		const auto RelRootEnd = rtm::vector_sub(RootPositions[LeftFoGFrames.second], LeftFootPositions[LeftFoGFrames.second]);
		RootDiff = rtm::vector_add(RootDiff, rtm::vector_sub(RelRootEnd, RelRootStart));

		FramesOnGround += (LeftFoGFrames.second >= LeftFoGFrames.first) ?
			(LeftFoGFrames.second - LeftFoGFrames.first) :
			(FrameCount - LeftFoGFrames.first + LeftFoGFrames.second + 1);
	}

	// ...and the same for the right foot
	const auto RightFoGFrames = FindFootOnGroundFrames(UpDir, RightFootPositions);
	if (RightFoGFrames.first < FrameCount)
	{
		Out.RightFootOnGroundFrame = static_cast<uint32_t>(RightFoGFrames.first);

		const auto RelRootStart = rtm::vector_sub(RootPositions[RightFoGFrames.first], RightFootPositions[RightFoGFrames.first]);
		const auto RelRootEnd = rtm::vector_sub(RootPositions[RightFoGFrames.second], RightFootPositions[RightFoGFrames.second]);
		RootDiff = rtm::vector_add(RootDiff, rtm::vector_sub(RelRootEnd, RelRootStart));

		FramesOnGround += (RightFoGFrames.second >= RightFoGFrames.first) ?
			(RightFoGFrames.second - RightFoGFrames.first) :
			(FrameCount - RightFoGFrames.first + RightFoGFrames.second + 1);
	}

	if (FramesOnGround)
	{
		//???or project RootDiff onto XZ plane? or store RootDiff as velocity instead of speed?
		const auto ForwardMovement = rtm::vector_mul(ForwardDir, rtm::vector_dot3(RootDiff, ForwardDir));
		Out.SpeedFromFeet = rtm::vector_length3(ForwardMovement) * FrameRate / static_cast<float>(FramesOnGround);
	}
	else
	{
		Out.SpeedFromFeet = 0.f;
	}

	// Try to extract averaged locomotion speed from the root motion.
	// Note that it can be not equal to SpeedFromFeet and may be even non-constant during the clip.
	{
		//???or project FullRootDiff onto XZ plane? or store FullRootDiff as velocity instead of speed?
		const auto FullRootDiff = rtm::vector_sub(RootPositions.back(), RootPositions.front());
		const auto ForwardMovement = rtm::vector_mul(ForwardDir, rtm::vector_dot3(FullRootDiff, ForwardDir));
		Out.SpeedFromRoot = rtm::vector_length3(ForwardMovement) * FrameRate / static_cast<float>(FrameCount);
	}

	return true;
}
//---------------------------------------------------------------------
