#include <ContentForgeTool.h>
#include <SceneTools.h>
#include <Utils.h>
#include <ParamsUtils.h>
#include <CLI11.hpp>
#include <GLTFSDK/GLTFResourceReader.h>
#include <GLTFSDK/GLBResourceReader.h>
#include <GLTFSDK/Deserialize.h>
#include <GLTFSDK/MeshPrimitiveUtils.h>
#include "GLTFExtensions.h"
#include <acl/core/ansi_allocator.h>
#include <acl/core/unique_ptr.h>
#include <acl/algorithm/uniformly_sampled/encoder.h>

namespace acl
{
	typedef std::unique_ptr<AnimationClip, Deleter<AnimationClip>> AnimationClipPtr;
	typedef std::unique_ptr<RigidSkeleton, Deleter<RigidSkeleton>> RigidSkeletonPtr;
}

namespace fs = std::filesystem;
namespace gltf = Microsoft::glTF;

// Set working directory to $(TargetDir)
// Example args:
// -s src/scenes

class CStreamReader : public gltf::IStreamReader
{
public:

	CStreamReader(fs::path&& pathBase) : _BasePath(std::move(pathBase)) {}

	std::shared_ptr<std::istream> GetInputStream(const std::string& FileName) const override
	{
		auto StreamPath = _BasePath / fs::u8path(FileName);
		auto Stream = std::make_shared<std::ifstream>(StreamPath, std::ios_base::binary);
		return (Stream && *Stream) ? Stream : nullptr;
	}

private:

	fs::path _BasePath;
};

class CGLTFTool : public CContentForgeTool
{
protected:

	struct CMeshAttrInfo
	{
		std::string MeshID;
		std::vector<std::string> MaterialIDs; // Per group (submesh)
	};

	struct CContext
	{
		CThreadSafeLog&           Log;

		gltf::Document Doc;
		std::unique_ptr<gltf::GLTFResourceReader> ResourceReader;

		acl::ANSIAllocator        ACLAllocator;
		acl::CompressionSettings  ACLSettings;

		fs::path                  MeshPath;
		fs::path                  SkinPath;
		fs::path                  AnimPath;
		std::string               TaskName;
		Data::CParamsSorted       MaterialMap;

		std::unordered_map<std::string, CMeshAttrInfo> ProcessedMeshes;
		std::unordered_map<std::string, std::string> ProcessedSkins;
	};

	struct CBone
	{
		//const FbxNode* pBone;
		gltf::Matrix4  InvLocalBindPose;
		std::string    ID;
		uint16_t       ParentBoneIndex;
		// TODO: bone object-space or local-space AABB
	};

	struct CSkeletonACLBinding
	{
		acl::RigidSkeletonPtr Skeleton;
		//std::vector<FbxNode*> FbxBones; // Indices in this array are used as bone indices in ACL
	};

	Data::CSchemeSet          _SceneSchemes;
	acl::TransformErrorMetric _ACLErrorMetric; // Stateless and therefore reusable

	std::string               _ResourceRoot;
	std::string               _SchemeFile;
	double                    _AnimSamplingRate = 30.0;
	bool                      _OutputBin = false;
	bool                      _OutputHRD = false; // For debug purposes, saves scene hierarchies in a human-readable format

public:

	CGLTFTool(const std::string& Name, const std::string& Desc, CVersion Version)
		: CContentForgeTool(Name, Desc, Version)
		, _ResourceRoot("Data:")
	{
		// Set default before parsing command line
		_RootDir = "../../../content";
		_SchemeFile = "../schemes/scene.dss";
	}

	virtual bool SupportsMultithreading() const override
	{
		// TODO: CStringID and multiple tasks writing to one output file
		return false;
	}

	virtual int Init() override
	{
		if (_ResourceRoot.empty())
			if (_LogVerbosity >= EVerbosity::Warnings)
				std::cout << "Resource root is empty, external references may not be resolved from the game!";

		if (!_OutputHRD) _OutputBin = true;

		if (_OutputBin)
		{
			if (!ParamsUtils::LoadSchemes(_SchemeFile.c_str(), _SceneSchemes))
			{
				std::cout << "Couldn't load scene binary serialization scheme from " << _SchemeFile;
				return 2;
			}
		}

		return 0;
	}

	virtual int Term() override
	{
		return 0;
	}

	virtual void ProcessCommandLine(CLI::App& CLIApp) override
	{
		CContentForgeTool::ProcessCommandLine(CLIApp);
		CLIApp.add_option("--res-root", _ResourceRoot, "Resource root prefix for referencing external subresources by path");
		CLIApp.add_option("--scheme,--schema", _SchemeFile, "Scene binary serialization scheme file path");
		CLIApp.add_option("--fps", _AnimSamplingRate, "Animation sampling rate in frames per second, default is 30");
		CLIApp.add_flag("-t,--txt", _OutputHRD, "Output scenes in a human-readable format, suitable for debugging only");
		CLIApp.add_flag("-b,--bin", _OutputBin, "Output scenes in a binary format, suitable for loading into the engine");
	}

	virtual bool ProcessTask(CContentForgeTask& Task) override
	{
		const auto Extension = Task.SrcFilePath.extension();
		const bool IsGLTF = (Extension == ".gltf");
		const bool IsGLB = (Extension == ".glb");
		if (!IsGLTF && !IsGLB)
		{
			Task.Log.LogWarning("Filename extension must be .gltf or .glb");
			return false;
		}

		// Open glTF document

		CContext Ctx{ Task.Log };

		auto StreamReader = std::make_unique<CStreamReader>(Task.SrcFilePath.parent_path());

		const auto SrcFileName = Task.SrcFilePath.filename().u8string();
		auto gltfStream = StreamReader->GetInputStream(SrcFileName);

		std::string Manifest;
		if (IsGLTF)
		{
			Ctx.ResourceReader = std::make_unique<gltf::GLTFResourceReader>(std::move(StreamReader));

			std::stringstream manifestStream;
			manifestStream << gltfStream->rdbuf();
			Manifest = manifestStream.str();
		}
		else
		{
			auto glbResourceReader = std::make_unique<gltf::GLBResourceReader>(std::move(StreamReader), std::move(gltfStream));
			Manifest = glbResourceReader->GetJson();
			Ctx.ResourceReader = std::move(glbResourceReader);
		}

		try
		{
			Ctx.Doc = gltf::Deserialize(Manifest, gltf::KHR::GetKHRExtensionDeserializer_DEM());
		}
		catch (const gltf::GLTFException& e)
		{
			Task.Log.LogError("Error deserializing glTF file " + SrcFileName + ":\n" + e.what());
			return false;
		}

		// Output file info

		Task.Log.LogInfo("Asset Version:    " + Ctx.Doc.asset.version);
		Task.Log.LogInfo("Asset MinVersion: " + Ctx.Doc.asset.minVersion);
		Task.Log.LogInfo("Asset Generator:  " + Ctx.Doc.asset.generator);
		Task.Log.LogInfo("Asset Copyright:  " + Ctx.Doc.asset.copyright + Task.Log.GetLineEnd());

		// Prepare other task context details

		//!!!TODO: need flags, what to export! command-line override must be provided along with .meta params

		Ctx.TaskName = Task.TaskID.CStr();
		// TODO: replace forbidden characters!

		fs::path OutPath = ParamsUtils::GetParam<std::string>(Task.Params, "Output", std::string{});
		if (!_RootDir.empty() && OutPath.is_relative())
			OutPath = fs::path(_RootDir) / OutPath;

		Ctx.MeshPath = ParamsUtils::GetParam<std::string>(Task.Params, "MeshOutput", std::string{});
		if (!_RootDir.empty() && Ctx.MeshPath.is_relative())
			Ctx.MeshPath = fs::path(_RootDir) / Ctx.MeshPath;

		Ctx.SkinPath = ParamsUtils::GetParam<std::string>(Task.Params, "SkinOutput", std::string{});
		if (!_RootDir.empty() && Ctx.SkinPath.is_relative())
			Ctx.SkinPath = fs::path(_RootDir) / Ctx.SkinPath;

		Ctx.AnimPath = ParamsUtils::GetParam<std::string>(Task.Params, "AnimOutput", std::string{});
		if (!_RootDir.empty() && Ctx.AnimPath.is_relative())
			Ctx.AnimPath = fs::path(_RootDir) / Ctx.AnimPath;

		// Export node hierarchy to DEM format

		if (Ctx.Doc.scenes.Size() > 1)
		{
			Task.Log.LogWarning("File contains multiple scenes, only default one will be exported!");
		}
		else if (Ctx.Doc.scenes.Size() < 1)
		{
			Task.Log.LogError("File contains no scenes!");
			return false;
		}

		const auto& Scene = Ctx.Doc.scenes[Ctx.Doc.defaultSceneId];
		
		Data::CParams Nodes;

		for (const auto& Node : Scene.nodes)
			if (!ExportNode(Node, Ctx, Nodes)) return false;

		// Export animations

		// Could also use acl::get_default_compression_settings()
		Ctx.ACLSettings.level = acl::CompressionLevel8::Medium;
		Ctx.ACLSettings.rotation_format = acl::RotationFormat8::QuatDropW_Variable;
		Ctx.ACLSettings.translation_format = acl::VectorFormat8::Vector3_Variable;
		Ctx.ACLSettings.scale_format = acl::VectorFormat8::Vector3_Variable;
		Ctx.ACLSettings.range_reduction = acl::RangeReductionFlags8::AllTracks;
		Ctx.ACLSettings.segmenting.enabled = true;
		Ctx.ACLSettings.segmenting.range_reduction = acl::RangeReductionFlags8::AllTracks;
		Ctx.ACLSettings.error_metric = &_ACLErrorMetric;

		//...

		// Export additional info

		// pScene->GetGlobalSettings().GetAmbientColor();
		// Scene->GetPoseCount();

		// Finalize and save the scene

		const std::string TaskName = Task.TaskID.ToString();

		Data::CParams Result;
		//if (Nodes.size() == 1)
		//{
		//	// TODO: Is it a good idea to save its only child as a root instead?
		//	Result = std::move(Nodes[0].second.GetValue<Data::CParams>());
		//}
		//else
		if (!Nodes.empty())
		{
			Result.emplace_back(CStrID("Children"), std::move(Nodes));
		}

		if (_OutputHRD)
		{
			const auto DestPath = OutPath / (TaskName + ".hrd");
			if (!ParamsUtils::SaveParamsToHRD(DestPath.string().c_str(), Result))
			{
				Task.Log.LogError("Error serializing " + TaskName + " to text");
				return false;
			}
		}

		if (_OutputBin)
		{
			const auto DestPath = OutPath / (Task.TaskID.ToString() + ".scn");
			if (!ParamsUtils::SaveParamsByScheme(DestPath.string().c_str(), Result, CStrID("SceneNode"), _SceneSchemes))
			{
				Task.Log.LogError("Error serializing " + TaskName + " to binary");
				return false;
			}
		}

		return true;
	}

	bool ExportNode(const std::string& NodeName, CContext& Ctx, Data::CParams& Nodes)
	{
		const auto& Node = Ctx.Doc.nodes[NodeName];

		Ctx.Log.LogDebug(std::string("Node ") + Node.name);

		static const CStrID sidTranslation("Translation");
		static const CStrID sidRotation("Rotation");
		static const CStrID sidScale("Scale");
		static const CStrID sidAttrs("Attrs");
		static const CStrID sidChildren("Children");

		Data::CParams NodeSection;

		// Process attributes

		Data::CDataArray Attributes;

		if (!Node.cameraId.empty())
			if (!ExportCamera(Node.cameraId, Ctx, Attributes)) return false;

		if (!Node.meshId.empty())
			if (!ExportModel(Node.meshId, Ctx, Attributes)) return false;

		if (!Node.skinId.empty())
			if (!ExportSkin(Node.skinId, Ctx, Attributes)) return false;

		{
			if (Node.HasExtension<gltf::KHR::Lights::NodeLightPunctual>())
			{
				const auto& LightsExt = Node.GetExtension<gltf::KHR::Lights::NodeLightPunctual>();
				if (!ExportLight(LightsExt.lightIndex, Ctx, Attributes)) return false;
			}
		}

		{
			auto ItLOD = Node.extensions.find("MSFT_lod");
			if (ItLOD != Node.extensions.cend())
			{
				assert(false && "IMPLEMENT ME!");
				Ctx.Log.LogDebug(ItLOD->first + " > " + ItLOD->second);
			}
		}

		if (!Attributes.empty())
			NodeSection.emplace_back(sidAttrs, std::move(Attributes));

		// Process transform

		// Bone transformation is determined by the bind pose and animations
		//...

		if (Node.matrix != gltf::Matrix4::IDENTITY)
		{
			const acl::AffineMatrix_32 ACLMatrix = acl::matrix_set(
				acl::Vector4_32{ Node.matrix.values[0], Node.matrix.values[1], Node.matrix.values[2], Node.matrix.values[3] },
				acl::Vector4_32{ Node.matrix.values[4], Node.matrix.values[5], Node.matrix.values[6], Node.matrix.values[7] },
				acl::Vector4_32{ Node.matrix.values[8], Node.matrix.values[9], Node.matrix.values[10], Node.matrix.values[11] },
				acl::Vector4_32{ 0.f, 0.f, 0.f, 1.f });

			const acl::Vector4_32 Scale = {
				acl::vector_length3(ACLMatrix.x_axis),
				acl::vector_length3(ACLMatrix.y_axis),
				acl::vector_length3(ACLMatrix.z_axis),
				0.f };

			const acl::Vector4_32 Translation = {
				Node.matrix.values[12],
				Node.matrix.values[13],
				Node.matrix.values[14],
				1.f };

			const acl::Quat_32 Rotation = acl::quat_from_matrix(acl::matrix_remove_scale(ACLMatrix));

			constexpr acl::Vector4_32 Unit3 = { 1.f, 1.f, 1.f, 0.f };
			constexpr acl::Vector4_32 Zero3 = { 0.f, 0.f, 0.f, 0.f };
			constexpr acl::Quat_32 IdentityQuat = { 0.f, 0.f, 0.f, 1.f };

			if (!acl::vector_all_near_equal3(Scale, Unit3))
				NodeSection.emplace_back(sidScale, vector4({ acl::vector_get_x(Scale), acl::vector_get_y(Scale), acl::vector_get_z(Scale) }));

			if (!acl::quat_near_equal(Rotation, IdentityQuat))
				NodeSection.emplace_back(sidRotation, vector4({ acl::quat_get_x(Rotation), acl::quat_get_y(Rotation), acl::quat_get_z(Rotation), acl::quat_get_w(Rotation) }));

			if (!acl::vector_all_near_equal3(Translation, Zero3))
				NodeSection.emplace_back(sidTranslation, vector4({ acl::vector_get_x(Translation), acl::vector_get_y(Translation), acl::vector_get_z(Translation) }));
		}
		else
		{
			if (Node.scale != gltf::Vector3::ONE)
				NodeSection.emplace_back(sidScale, vector4({ Node.scale.x, Node.scale.y, Node.scale.z }));
			if (Node.rotation != gltf::Quaternion::IDENTITY)
				NodeSection.emplace_back(sidRotation, vector4({ Node.rotation.x, Node.rotation.y, Node.rotation.z, Node.rotation.w }));
			if (Node.translation != gltf::Vector3::ZERO)
				NodeSection.emplace_back(sidTranslation, vector4({ Node.translation.x, Node.translation.y, Node.translation.z }));
		}

		// Process children

		Data::CParams Children;

		for (const auto& Child : Node.children)
			if (!ExportNode(Child, Ctx, Children)) return false;

		if (!Children.empty())
			NodeSection.emplace_back(sidChildren, std::move(Children));

		CStrID NodeID = CStrID(Node.name.c_str());
		if (ParamsUtils::HasParam(Nodes, NodeID))
			Ctx.Log.LogWarning("Duplicated node overwritten with name " + Node.name);

		Nodes.emplace_back(NodeID, std::move(NodeSection));

		return true;
	}

	bool ExportModel(const std::string& MeshName, CContext& Ctx, Data::CDataArray& Attributes)
	{
		const auto& Mesh = Ctx.Doc.meshes[MeshName];

		Ctx.Log.LogDebug("Model " + Mesh.name);

		// Export mesh (optionally skinned)

		auto MeshIt = Ctx.ProcessedMeshes.find(MeshName);
		if (MeshIt == Ctx.ProcessedMeshes.cend())
		{
			if (!ExportMesh(MeshName, Ctx)) return false;
			MeshIt = Ctx.ProcessedMeshes.find(MeshName);
			if (MeshIt == Ctx.ProcessedMeshes.cend()) return false;
		}

		const CMeshAttrInfo& MeshInfo = MeshIt->second;

		// Add models per mesh group

		int GroupIndex = 0;
		for (const auto& MaterialID : MeshInfo.MaterialIDs)
		{
			Data::CParams ModelAttribute;
			ModelAttribute.emplace_back(CStrID("Class"), std::string("Frame::CModelAttribute"));
			ModelAttribute.emplace_back(CStrID("Mesh"), MeshInfo.MeshID);
			if (GroupIndex > 0)
				ModelAttribute.emplace_back(CStrID("MeshGroupIndex"), GroupIndex);
			ModelAttribute.emplace_back(CStrID("Material"), MaterialID);
			Attributes.push_back(std::move(ModelAttribute));

			++GroupIndex;
		}

		return true;
	}

	bool ExportMesh(const std::string& MeshName, CContext& Ctx)
	{
		const auto& Mesh = Ctx.Doc.meshes[MeshName];

		CVertexFormat VertexFormat;
		int BoneCount = 0;
		constexpr const char* UVAttributes[] = { gltf::ACCESSOR_TEXCOORD_0, gltf::ACCESSOR_TEXCOORD_1 };

		// Extract mesh groups (submeshes)

		std::map<std::string, CMeshData> SubMeshes;

		for (const auto& Primitive : Mesh.primitives)
		{
			// Extract vertex data from glTF

			CMeshData& SubMesh = SubMeshes.emplace(Primitive.materialId, CMeshData{}).first->second;
			std::string AccessorId;

			if (!Primitive.TryGetAttributeAccessorId(gltf::ACCESSOR_POSITION, AccessorId))
			{
				Ctx.Log.LogWarning("Submesh " + Mesh.name + '.' + Primitive.materialId + " has no vertex positions, skipped");
				continue;
			}

			std::vector<CVertex> RawVertices;

			{
				const auto& Accessor = Ctx.Doc.accessors[AccessorId];

				const auto Positions = gltf::MeshPrimitiveUtils::GetPositions(Ctx.Doc, *Ctx.ResourceReader, Accessor);

				RawVertices.resize(Positions.size() / 3);

				auto AttrIt = Positions.cbegin();
				for (auto& Vertex : RawVertices)
				{
					Vertex.Position.x = *AttrIt++;
					Vertex.Position.y = *AttrIt++;
					Vertex.Position.z = *AttrIt++;
				}

				SubMesh.AABBMin.x = Accessor.min[0];
				SubMesh.AABBMin.y = Accessor.min[1];
				SubMesh.AABBMin.z = Accessor.min[2];
				SubMesh.AABBMax.x = Accessor.max[0];
				SubMesh.AABBMax.y = Accessor.max[1];
				SubMesh.AABBMax.z = Accessor.max[2];
			}

			if (Primitive.TryGetAttributeAccessorId(gltf::ACCESSOR_NORMAL, AccessorId))
			{
				const auto Normals = gltf::MeshPrimitiveUtils::GetNormals(Ctx.Doc, *Ctx.ResourceReader, Ctx.Doc.accessors[AccessorId]);

				auto AttrIt = Normals.cbegin();
				for (auto& Vertex : RawVertices)
				{
					Vertex.Normal.x = *AttrIt++;
					Vertex.Normal.y = *AttrIt++;
					Vertex.Normal.z = *AttrIt++;
				}

				++VertexFormat.NormalCount;
			}

			if (Primitive.TryGetAttributeAccessorId(gltf::ACCESSOR_TANGENT, AccessorId))
			{
				const auto Tangents = gltf::MeshPrimitiveUtils::GetTangents(Ctx.Doc, *Ctx.ResourceReader, Ctx.Doc.accessors[AccessorId]);

				auto AttrIt = Tangents.cbegin();
				for (auto& Vertex : RawVertices)
				{
					Vertex.Tangent.x = *AttrIt++;
					Vertex.Tangent.y = *AttrIt++;
					Vertex.Tangent.z = *AttrIt++;

					// Calculate bitangent. See implementation details at:
					// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#meshes

					const float Sign = *AttrIt++;

					const acl::Vector4_32 Normal = { Vertex.Normal.x, Vertex.Normal.y, Vertex.Normal.z };
					const acl::Vector4_32 Tangent = { Vertex.Tangent.x, Vertex.Tangent.y, Vertex.Tangent.z };
					const acl::Vector4_32 Bitangent = acl::vector_cross3(Normal, Tangent);

					Vertex.Bitangent.x = acl::vector_get_x(Bitangent) * Sign;
					Vertex.Bitangent.y = acl::vector_get_y(Bitangent) * Sign;
					Vertex.Bitangent.z = acl::vector_get_z(Bitangent) * Sign;
				}

				++VertexFormat.TangentCount;
				++VertexFormat.BitangentCount;
			}

			if (Primitive.TryGetAttributeAccessorId(gltf::ACCESSOR_COLOR_0, AccessorId))
			{
				const auto Colors = gltf::MeshPrimitiveUtils::GetColors(Ctx.Doc, *Ctx.ResourceReader, Ctx.Doc.accessors[AccessorId]);

				auto AttrIt = Colors.cbegin();
				for (auto& Vertex : RawVertices)
					Vertex.Color = *AttrIt++;

				++VertexFormat.ColorCount;
			}

			for (int UVIdx = 0; UVIdx < MaxUV; ++UVIdx)
			{
				if (Primitive.TryGetAttributeAccessorId(UVAttributes[UVIdx], AccessorId))
				{
					// Verify that mesh has no gaps in the UV attributes sequence
					assert(VertexFormat.UVCount == UVIdx);

					const auto UV = gltf::MeshPrimitiveUtils::GetTexCoords(Ctx.Doc, *Ctx.ResourceReader, Ctx.Doc.accessors[AccessorId]);

					auto AttrIt = UV.cbegin();
					for (auto& Vertex : RawVertices)
					{
						Vertex.UV[VertexFormat.UVCount].x = *AttrIt++;
						Vertex.UV[VertexFormat.UVCount].y = *AttrIt++;
					}

					++VertexFormat.UVCount;
				}
			}

			if (Primitive.TryGetAttributeAccessorId(gltf::ACCESSOR_JOINTS_0, AccessorId))
			{
				const auto Joints = gltf::MeshPrimitiveUtils::GetJointIndices64(Ctx.Doc, *Ctx.ResourceReader, Ctx.Doc.accessors[AccessorId]);

				auto AttrIt = Joints.cbegin();
				for (auto& Vertex : RawVertices)
				{
					const uint64_t Packed = *AttrIt++;
					Vertex.BlendIndices[0] = ((Packed >>  0) & 0xffff);
					Vertex.BlendIndices[1] = ((Packed >> 16) & 0xffff);
					Vertex.BlendIndices[2] = ((Packed >> 32) & 0xffff);
					Vertex.BlendIndices[3] = ((Packed >> 48) & 0xffff);

					for (size_t i = 0; i < 4; ++i)
					{
						Vertex.BlendIndices[i] = ((Packed >> (i * 16)) & 0xffff);
						if (BoneCount <= Vertex.BlendIndices[i])
							BoneCount = Vertex.BlendIndices[i] + 1;
					}
				}

				VertexFormat.BonesPerVertex = 4;
			}

			if (Primitive.TryGetAttributeAccessorId(gltf::ACCESSOR_WEIGHTS_0, AccessorId))
			{
				// Must have joint indices
				assert(VertexFormat.BonesPerVertex == 4);

				//!!!check weights format!
				assert(false);

				const auto Joints = gltf::MeshPrimitiveUtils::GetJointWeights32(Ctx.Doc, *Ctx.ResourceReader, Ctx.Doc.accessors[AccessorId]);

				auto AttrIt = Joints.cbegin();
				for (auto& Vertex : RawVertices)
				{
					Vertex.BlendWeights = *AttrIt++;

					// TODO: store 4 float weights? Then get floats from accessor, don't pack with GetJointWeights32.
					// Or store uint32_t weights, then can store as is here, don't unpack!
					//const uint32_t Packed = *AttrIt++;
					//Vertex.BlendWeights[0] = gltf::Math::ByteToFloat(Packed & 0xffff);
					//Vertex.BlendWeights[1] = gltf::Math::ByteToFloat((Packed >> 16) & 0xffff);
					//Vertex.BlendWeights[2] = gltf::Math::ByteToFloat((Packed >> 32) & 0xffff);
					//Vertex.BlendWeights[3] = gltf::Math::ByteToFloat((Packed >> 48) & 0xffff);
				}
			}

			// Extract index data from glTF
			// NB: glTF indices are CCW

			std::vector<uint32_t> RawIndices;
			if (!Primitive.indicesAccessorId.empty())
				RawIndices = std::move(gltf::MeshPrimitiveUtils::GetIndices32(Ctx.Doc, *Ctx.ResourceReader, Primitive));

			// Optimize vertices and indices

			ProcessGeometry(RawVertices, RawIndices, SubMesh.Vertices, SubMesh.Indices);
		}

		// Write resulting mesh file

		{
			// TODO: replace forbidden characters (std::transform with replacer callback?)
			//???use node name when possible?
			std::string MeshRsrcName = Mesh.name.empty() ? Ctx.TaskName + '_' + MeshName : Mesh.name;
			ToLower(MeshRsrcName);

			const auto DestPath = Ctx.MeshPath / (MeshRsrcName + ".msh");

			if (!WriteDEMMesh(DestPath, SubMeshes, VertexFormat, static_cast<size_t>(BoneCount), Ctx.Log)) return false;

			CMeshAttrInfo MeshInfo;
			MeshInfo.MeshID = _ResourceRoot + fs::relative(DestPath, _RootDir).generic_string();
			for (const auto& Pair : SubMeshes)
				MeshInfo.MaterialIDs.push_back(Pair.first);

			Ctx.ProcessedMeshes.emplace(MeshName, std::move(MeshInfo));
		}

		return true;
	}

	bool ExportSkin(const std::string& SkinName, CContext& Ctx, Data::CDataArray& Attributes)
	{
		const auto& Skin = Ctx.Doc.skins[SkinName];

		Ctx.Log.LogDebug(std::string("Skin ") + Skin.name);

		return true;
	}

	bool ExportCamera(const std::string& CameraName, CContext& Ctx, Data::CDataArray& Attributes)
	{
		const auto& Camera = Ctx.Doc.cameras[CameraName];

		Ctx.Log.LogDebug(std::string("Camera ") + Camera.name);

		Data::CParams Attribute;
		Attribute.emplace_back(CStrID("Class"), std::string("Frame::CCameraAttribute"));

		if (const auto* pOrthographic = dynamic_cast<gltf::Orthographic*>(Camera.projection.get()))
		{
			Attribute.emplace_back(CStrID("Orthogonal"), true);

			Attribute.emplace_back(CStrID("NearPlane"), pOrthographic->znear);
			Attribute.emplace_back(CStrID("FarPlane"), pOrthographic->zfar);

			assert(false && "IMPLEMENT!");
			//pOrthographic->xmag;
			//pOrthographic->ymag;
		}
		else if (const auto* pPerspective = dynamic_cast<gltf::Perspective*>(Camera.projection.get()))
		{
			Attribute.emplace_back(CStrID("FOV"), RadToDeg(pPerspective->yfov));

			if (pPerspective->HasCustomAspectRatio())
			{
				// FIXME: aspect instead of W&H?
				//Attribute.emplace_back(CStrID("Aspect"), pPerspective->aspectRatio.Get());

				constexpr float DefaultHeight = 1024.f;
				Attribute.emplace_back(CStrID("Width"), DefaultHeight * pPerspective->aspectRatio.Get());
				Attribute.emplace_back(CStrID("Height"), DefaultHeight);
			}

			Attribute.emplace_back(CStrID("NearPlane"), pPerspective->znear);

			if (pPerspective->IsFinite())
				Attribute.emplace_back(CStrID("FarPlane"), pPerspective->zfar.Get());
		}
		else
		{
			Ctx.Log.LogError("Unknown camera projection type!");
			return false;
		}

		Attributes.push_back(std::move(Attribute));

		return true;
	}

	// https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_lights_punctual
	bool ExportLight(int LightIndex, CContext& Ctx, Data::CDataArray& Attributes)
	{
		if (!Ctx.Doc.HasExtension<gltf::KHR::Lights::LightsPunctual>())
		{
			Ctx.Log.LogError("KHR_lights_punctual reference exists in a node but no extension present in the document");
			return false;
		}

		const auto& LightsExt = Ctx.Doc.GetExtension<gltf::KHR::Lights::LightsPunctual>();
		if (static_cast<int>(LightsExt.lights.size()) <= LightIndex || LightIndex < 0)
		{
			Ctx.Log.LogError("Invalid KHR_lights_punctual light index " + std::to_string(LightIndex));
			return false;
		}

		const auto& Light = LightsExt.lights[LightIndex];

		int LightType = 0;
		switch (Light.type)
		{
			case gltf::KHR::Lights::Type::Directional: LightType = 0; break;
			case gltf::KHR::Lights::Type::Point: LightType = 1; break;
			case gltf::KHR::Lights::Type::Spot: LightType = 2; break;
			default:
			{
				Ctx.Log.LogWarning(std::string("Light ") + Light.name + " is of unsupported type, skipped");
				return true;
			}
		}

		// TODO: choose correct base! glTF uses physical units for the light intensity and we need relative value.
		constexpr float BaseIntensity = 100.f;
		const float Intensity = Light.intensity / BaseIntensity;

		Data::CParams Attribute;
		Attribute.emplace_back(CStrID("Class"), std::string("Frame::CLightAttribute"));
		Attribute.emplace_back(CStrID("LightType"), LightType);
		Attribute.emplace_back(CStrID("Color"), vector4({ Light.color.r, Light.color.g, Light.color.b }));
		Attribute.emplace_back(CStrID("Intensity"), Intensity);

		if (LightType != 0)
		{
			// attenuation = max( min( 1.0 - ( current_distance / range )^4, 1 ), 0 ) / current_distance^2
			if (Light.range.HasValue())
			{
				Attribute.emplace_back(CStrID("Range"), Light.range.Get());
			}
			else
			{
				// Range is calculated based on link below with intensity in [0; 1]
				// https://developer.valvesoftware.com/wiki/Constant-Linear-Quadratic_Falloff
				Attribute.emplace_back(CStrID("Range"), std::sqrtf(Light.intensity) * 100.f);
			}

			if (LightType == 2)
			{
				Attribute.emplace_back(CStrID("ConeInner"), RadToDeg(Light.innerConeAngle));
				Attribute.emplace_back(CStrID("ConeOuter"), RadToDeg(Light.outerConeAngle));
			}
		}

		Attributes.push_back(std::move(Attribute));

		return true;
	}
};

int main(int argc, const char** argv)
{
	CGLTFTool Tool("cf-gltf", "glTF 2.0 to DeusExMachina resource converter", { 0, 1, 0 });
	return Tool.Execute(argc, argv);
}
