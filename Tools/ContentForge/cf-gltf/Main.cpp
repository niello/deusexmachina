#include <ContentForgeTool.h>
#include <SceneTools.h>
#include <Render/ShaderMetaCommon.h>
#include <Utils.h>
#include <ParamsUtils.h>
#include <CLI11.hpp>
#include <GLTFSDK/GLTFResourceReader.h>
#include <GLTFSDK/GLBResourceReader.h>
#include <GLTFSDK/Deserialize.h>
#include <GLTFSDK/MeshPrimitiveUtils.h>
#include <GLTFSDK/AnimationUtils.h>
#include "GLTFExtensions.h"
#include <acl/core/ansi_allocator.h>
#include <acl/compression/animation_clip.h>

namespace fs = std::filesystem;
namespace gltf = Microsoft::glTF;

// Set working directory to $(TargetDir)
// Example args:
// -s src/scenes --path Data ../../../content

static const gltf::Node* GetParentNode(const gltf::Document& Doc, const std::string& NodeID)
{
	for (const auto& Node: Doc.nodes.Elements())
	{
		const auto& Children = Node.children;
		if (std::find(Children.cbegin(), Children.cend(), NodeID) != Children.cend())
			return &Node;
	}

	return nullptr;
}
//---------------------------------------------------------------------

class CStreamReader : public gltf::IStreamReader
{
public:

	explicit CStreamReader(const fs::path& BasePath) : _BasePath(BasePath) {}

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

	struct CContext
	{
		CThreadSafeLog&           Log;

		gltf::Document Doc;
		std::unique_ptr<gltf::GLTFResourceReader> ResourceReader;

		acl::ANSIAllocator        ACLAllocator;

		fs::path                  SrcFolder;
		fs::path                  MeshPath;
		fs::path                  MaterialPath;
		fs::path                  TexturePath;
		fs::path                  SkinPath;
		fs::path                  AnimPath;
		std::string               TaskName;

		std::unordered_map<std::string, CMeshAttrInfo> ProcessedMeshes;
		std::unordered_map<std::string, std::string> ProcessedMaterials;
		std::unordered_map<std::string, std::string> ProcessedTextures;
		std::unordered_map<std::string, std::string> ProcessedSkins;
	};

	struct CSkeletonACLBinding
	{
		acl::RigidSkeletonPtr Skeleton;
		std::vector<const gltf::Node*> Nodes; // Indices in this array are used as bone indices in ACL
	};

	Data::CSchemeSet          _SceneSchemes;

	std::map<std::string, std::string> _EffectsByType;
	std::map<std::string, std::string> _EffectParamAliases;

	std::string               _ResourceRoot;
	std::string               _SchemeFile;
	std::string               _SettingsFile;
	float                     _AnimSamplingRate = 30.f;
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
		_SettingsFile = "../schemes/settings.hrd";
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

		{
			Data::CParams EffectSettings;
			if (!ParamsUtils::LoadParamsFromHRD(_SettingsFile.c_str(), EffectSettings))
			{
				std::cout << "Couldn't load effect settings from " << _SettingsFile;
				return 3;
			}

			const Data::CParams* pMap;
			if (ParamsUtils::TryGetParam(pMap, EffectSettings, "Effects"))
			{
				for (const auto& Pair : *pMap)
					_EffectsByType.emplace(Pair.first.ToString(), Pair.second.GetValue<std::string>());
			}
			if (ParamsUtils::TryGetParam(pMap, EffectSettings, "Params"))
			{
				for (const auto& Pair : *pMap)
					_EffectParamAliases.emplace(Pair.first.ToString(), Pair.second.GetValue<std::string>());
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
		//???use --project-file instead of --res-root + --settings?
		CContentForgeTool::ProcessCommandLine(CLIApp);
		CLIApp.add_option("--res-root", _ResourceRoot, "Resource root prefix for referencing external subresources by path");
		CLIApp.add_option("--scheme,--schema", _SchemeFile, "Scene binary serialization scheme file path");
		CLIApp.add_option("--settings", _SettingsFile, "Settings file path");
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

		Ctx.SrcFolder = Task.SrcFilePath.parent_path();

		auto StreamReader = std::make_unique<CStreamReader>(Ctx.SrcFolder);

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

		const std::string OutPathStr = OutPath.generic_string();
		std::string PathValue;

		if (ParamsUtils::TryGetParam(PathValue, Task.Params, "MeshOutput"))
		{
			Ctx.MeshPath = PathValue;
			if (!_RootDir.empty() && Ctx.MeshPath.is_relative())
				Ctx.MeshPath = fs::path(_RootDir) / Ctx.MeshPath;
		}
		else Ctx.MeshPath = OutPathStr;

		if (ParamsUtils::TryGetParam(PathValue, Task.Params, "MaterialOutput"))
		{
			Ctx.MaterialPath = PathValue;
			if (!_RootDir.empty() && Ctx.MaterialPath.is_relative())
				Ctx.MaterialPath = fs::path(_RootDir) / Ctx.MaterialPath;
		}
		else Ctx.MaterialPath = OutPathStr;

		if (ParamsUtils::TryGetParam(PathValue, Task.Params, "TextureOutput"))
		{
			Ctx.TexturePath = PathValue;
			if (!_RootDir.empty() && Ctx.TexturePath.is_relative())
				Ctx.TexturePath = fs::path(_RootDir) / Ctx.TexturePath;
		}
		else Ctx.TexturePath = OutPathStr;

		if (ParamsUtils::TryGetParam(PathValue, Task.Params, "SkinOutput"))
		{
			Ctx.SkinPath = PathValue;
			if (!_RootDir.empty() && Ctx.SkinPath.is_relative())
				Ctx.SkinPath = fs::path(_RootDir) / Ctx.SkinPath;
		}
		else Ctx.SkinPath = OutPathStr;

		if (ParamsUtils::TryGetParam(PathValue, Task.Params, "AnimOutput"))
		{
			Ctx.AnimPath = PathValue;
			if (!_RootDir.empty() && Ctx.AnimPath.is_relative())
				Ctx.AnimPath = fs::path(_RootDir) / Ctx.AnimPath;
		}
		else Ctx.AnimPath = OutPathStr;

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

		for (const auto& Anim : Ctx.Doc.animations.Elements())
			if (!ExportAnimation(Anim, Ctx)) return false;

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
		{
			std::string SkinID;
			if (!ExportSkin(Node.skinId, SkinID, Ctx)) return false;

			Data::CParams SkinAttribute;
			SkinAttribute.emplace_back(CStrID("Class"), std::string("Frame::CSkinAttribute"));
			SkinAttribute.emplace_back(CStrID("SkinInfo"), SkinID);
			//SkinAttribute.emplace(CStrID("AutocreateBones"), true);
			Attributes.push_back(std::move(SkinAttribute));
		}

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

		CStrID NodeID = CStrID(Node.name.empty() ? Ctx.TaskName + '_' + Node.id : Node.name.c_str());
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
		VertexFormat.FloatBlendWeights = false;
		int BoneCount = 0;

		constexpr const char* UVAttributes[] = { gltf::ACCESSOR_TEXCOORD_0, gltf::ACCESSOR_TEXCOORD_1 };
		constexpr size_t glTFMaxUV = (sizeof(UVAttributes) / sizeof(UVAttributes[0]));

		// Extract mesh groups (submeshes)

		std::map<std::string, CMeshGroup> SubMeshes;

		for (const auto& Primitive : Mesh.primitives)
		{
			// Extract vertex data from glTF

			auto& SubMesh = SubMeshes.emplace(Primitive.materialId, CMeshGroup{}).first->second;
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

			for (int UVIdx = 0; UVIdx < glTFMaxUV; ++UVIdx)
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

				const auto Weights = gltf::MeshPrimitiveUtils::GetJointWeights32(Ctx.Doc, *Ctx.ResourceReader, Ctx.Doc.accessors[AccessorId]);

				auto AttrIt = Weights.cbegin();
				for (auto& Vertex : RawVertices)
				{
					if (VertexFormat.FloatBlendWeights)
					{
						const uint32_t Packed = *AttrIt++;
						Vertex.BlendWeightsF[0] = gltf::Math::ByteToFloat((Packed >>  0) & 0xff);
						Vertex.BlendWeightsF[1] = gltf::Math::ByteToFloat((Packed >>  8) & 0xff);
						Vertex.BlendWeightsF[2] = gltf::Math::ByteToFloat((Packed >> 16) & 0xff);
						Vertex.BlendWeightsF[3] = gltf::Math::ByteToFloat((Packed >> 24) & 0xff);
					}
					else
					{
						Vertex.BlendWeightsI = *AttrIt++;
					}
				}
			}

			// Extract index data from glTF
			// NB: glTF indices are CCW

			std::vector<uint32_t> RawIndices;
			if (!Primitive.indicesAccessorId.empty())
				RawIndices = std::move(gltf::MeshPrimitiveUtils::GetTriangulatedIndices32(Ctx.Doc, *Ctx.ResourceReader, Primitive));

			// Optimize vertices and indices

			ProcessGeometry(RawVertices, RawIndices, SubMesh.Vertices, SubMesh.Indices);
		}

		// Write resulting mesh file

		//???use node name when possible?
		const auto MeshRsrcName = GetValidResourceName(Mesh.name.empty() ? Ctx.TaskName + '_' + MeshName : Mesh.name);

		const auto DestPath = Ctx.MeshPath / (MeshRsrcName + ".msh");

		if (!WriteDEMMesh(DestPath, SubMeshes, VertexFormat, static_cast<size_t>(BoneCount), Ctx.Log)) return false;

		// Export materials

		CMeshAttrInfo MeshInfo;
		MeshInfo.MeshID = _ResourceRoot + fs::relative(DestPath, _RootDir).generic_string();
		for (const auto& Pair : SubMeshes)
		{
			std::string MaterialID;
			if (!ExportMaterial(Pair.first, MaterialID, Ctx)) return false;
			MeshInfo.MaterialIDs.push_back(std::move(MaterialID));
		}

		Ctx.ProcessedMeshes.emplace(MeshName, std::move(MeshInfo));

		return true;
	}

	const std::string& GetEffectParamID(const std::string& Alias)
	{
		auto It = _EffectParamAliases.find(Alias);
		return (It == _EffectParamAliases.cend()) ? Alias : It->second;
	}

	// TODO: there is SGToMR in glTF SDK PBRUtils, can add SG source support through conversion to MR
	// Or just add a requirement for DEM assets to be authored in MR model.
	bool ExportMaterial(const std::string& MtlName, std::string& OutMaterialID, CContext& Ctx)
	{
		auto MtlIt = Ctx.ProcessedMaterials.find(MtlName);
		if (MtlIt != Ctx.ProcessedMaterials.cend())
		{
			OutMaterialID = MtlIt->second;
			return true;
		}

		const auto& Mtl = Ctx.Doc.materials[MtlName];

		Ctx.Log.LogDebug("Material " + Mtl.name);

		// Build abstract effect type ID

		std::string EffectTypeID = "MetallicRoughness";
		switch (Mtl.alphaMode)
		{
			case gltf::AlphaMode::ALPHA_BLEND: EffectTypeID += "Alpha"; break;
			case gltf::AlphaMode::ALPHA_MASK: EffectTypeID += "AlphaTest"; break;
			case gltf::AlphaMode::ALPHA_OPAQUE: EffectTypeID += "Opaque"; break;
			default:
			{
				// Additive KHR_blend: https://github.com/KhronosGroup/glTF/pull/1302
				// May be additive must be hardcoded for particles.
				Ctx.Log.LogError("glTF material " + Mtl.name + " has unknown alpha mode!");
				return false;
			}
		}

		EffectTypeID += Mtl.doubleSided ? "DoubleSided" : "Culled";

		// Get effect resource ID and material table from the effect file

		auto EffectIt = _EffectsByType.find(EffectTypeID);
		if (EffectIt == _EffectsByType.cend() || EffectIt->second.empty())
		{
			Ctx.Log.LogError("glTF material " + Mtl.name + " with type " + EffectTypeID + " has no mapped DEM effect file in effect settings");
			return false;
		}

		CMaterialParams MtlParamTable;
		auto Path = ResolvePathAliases(EffectIt->second).generic_string();
		Ctx.Log.LogDebug("Opening effect " + Path);
		if (!GetEffectMaterialParams(MtlParamTable, Path, Ctx.Log)) return false;

		Data::CParams MtlParams;

		// Fill material constants

		const auto& AlbedoFactorID = GetEffectParamID("AlbedoFactor");
		if (MtlParamTable.HasConstant(AlbedoFactorID))
		{
			const auto& AlbedoFactor = Mtl.metallicRoughness.baseColorFactor;
			if (AlbedoFactor != gltf::Color4(1.f, 1.f, 1.f, 1.f))
				MtlParams.emplace_back(CStrID(AlbedoFactorID), vector4(AlbedoFactor.r, AlbedoFactor.g, AlbedoFactor.b, AlbedoFactor.a));
		}

		const auto& MetallicFactorID = GetEffectParamID("MetallicFactor");
		if (MtlParamTable.HasConstant(MetallicFactorID))
		{
			if (Mtl.metallicRoughness.metallicFactor != 1.f)
				MtlParams.emplace_back(MetallicFactorID, Mtl.metallicRoughness.metallicFactor);
		}

		const auto& RoughnessFactorID = GetEffectParamID("RoughnessFactor");
		if (MtlParamTable.HasConstant(RoughnessFactorID))
		{
			if (Mtl.metallicRoughness.roughnessFactor != 1.f)
				MtlParams.emplace_back(RoughnessFactorID, Mtl.metallicRoughness.roughnessFactor);
		}

		const auto& EmissiveFactorID = GetEffectParamID("EmissiveFactor");
		if (MtlParamTable.HasConstant(EmissiveFactorID))
		{
			if (Mtl.emissiveFactor != gltf::Color3(0.f, 0.f, 0.f))
				MtlParams.emplace_back(EmissiveFactorID, vector4(Mtl.emissiveFactor.r, Mtl.emissiveFactor.g, Mtl.emissiveFactor.b, 0.f));
		}

		if (Mtl.alphaMode == gltf::AlphaMode::ALPHA_MASK)
		{
			const auto& AlphaCutoffID = GetEffectParamID("AlphaCutoff");
			if (MtlParamTable.HasConstant(AlphaCutoffID))
			{
				if (Mtl.alphaCutoff != 0.5f)
					MtlParams.emplace_back(AlphaCutoffID, Mtl.alphaCutoff);
			}
		}

		// Fill material textures and samplers

		std::set<std::string> GLTFSamplers;

		const auto& AlbedoTextureID = GetEffectParamID("AlbedoTexture");
		if (!Mtl.metallicRoughness.baseColorTexture.textureId.empty() && MtlParamTable.HasResource(AlbedoTextureID))
		{
			std::string TextureID;
			if (!ExportTexture(Mtl.metallicRoughness.baseColorTexture, TextureID, GLTFSamplers, Ctx)) return false;
			MtlParams.emplace_back(AlbedoTextureID, TextureID);
		}

		const auto& MetallicRoughnessTextureID = GetEffectParamID("MetallicRoughnessTexture");
		if (!Mtl.metallicRoughness.metallicRoughnessTexture.textureId.empty() && MtlParamTable.HasResource(MetallicRoughnessTextureID))
		{
			std::string TextureID;
			if (!ExportTexture(Mtl.metallicRoughness.metallicRoughnessTexture, TextureID, GLTFSamplers, Ctx)) return false;
			MtlParams.emplace_back(MetallicRoughnessTextureID, TextureID);
		}

		const auto& NormalTextureID = GetEffectParamID("NormalTexture");
		if (!Mtl.normalTexture.textureId.empty() && MtlParamTable.HasResource(NormalTextureID))
		{
			std::string TextureID;
			if (!ExportTexture(Mtl.normalTexture, TextureID, GLTFSamplers, Ctx)) return false;
			MtlParams.emplace_back(NormalTextureID, TextureID);
		}

		const auto& OcclusionTextureID = GetEffectParamID("OcclusionTexture");
		if (!Mtl.occlusionTexture.textureId.empty() && MtlParamTable.HasResource(OcclusionTextureID))
		{
			std::string TextureID;
			if (!ExportTexture(Mtl.occlusionTexture, TextureID, GLTFSamplers, Ctx)) return false;
			MtlParams.emplace_back(OcclusionTextureID, TextureID);
		}

		const auto& EmissiveTextureID = GetEffectParamID("EmissiveTexture");
		if (!Mtl.emissiveTexture.textureId.empty() && MtlParamTable.HasResource(EmissiveTextureID))
		{
			std::string TextureID;
			if (!ExportTexture(Mtl.emissiveTexture, TextureID, GLTFSamplers, Ctx)) return false;
			MtlParams.emplace_back(EmissiveTextureID, TextureID);
		}

		// FIXME: if no sampler, may need to save glTF-default sampler with repeat wrapping and auto-filtering
		//!!!TODO: if effect's default sampler is the same as material sampler, don't create another sampler in engine when loading material,
		//even if the material has the sampler explicitly defined! Compare CSamplerDesc.
		if (!GLTFSamplers.empty())
		{
			std::string SamplerName;
			if (GLTFSamplers.size() > 1)
				Ctx.Log.LogWarning("Material " + Mtl.name + " uses more than one sampler, but DEM supports only one sampler per PBR material");

			const auto& PBRTextureSamplerID = GetEffectParamID("PBRTextureSampler");
			if (MtlParamTable.HasSampler(PBRTextureSamplerID))
			{
				const std::string& SamplerGLTFID = *GLTFSamplers.begin();
				const auto& Sampler = Ctx.Doc.samplers[SamplerGLTFID];

				Data::CParams SamplerDesc;

				switch (Sampler.wrapS)
				{
					case gltf::WrapMode::Wrap_REPEAT: SamplerDesc.emplace_back(CStrID("AddressU"), std::string("wrap")); break;
					case gltf::WrapMode::Wrap_MIRRORED_REPEAT: SamplerDesc.emplace_back(CStrID("AddressU"), std::string("mirror")); break;
					case gltf::WrapMode::Wrap_CLAMP_TO_EDGE: SamplerDesc.emplace_back(CStrID("AddressU"), std::string("clamp")); break;
				}

				switch (Sampler.wrapT)
				{
					case gltf::WrapMode::Wrap_REPEAT: SamplerDesc.emplace_back(CStrID("AddressV"), std::string("wrap")); break;
					case gltf::WrapMode::Wrap_MIRRORED_REPEAT: SamplerDesc.emplace_back(CStrID("AddressV"), std::string("mirror")); break;
					case gltf::WrapMode::Wrap_CLAMP_TO_EDGE: SamplerDesc.emplace_back(CStrID("AddressV"), std::string("clamp")); break;
				}

				// NB: glTF 2.0 doesn't support anisotropic filtering. DEM should enable it through renderer settings,
				//     replacing exported sampler filter with anisotropic one.
				const bool IsMagPoint = Sampler.magFilter.HasValue() && Sampler.magFilter.Get() == gltf::MagFilterMode::MagFilter_NEAREST;
				auto MinFilter = Sampler.minFilter.HasValue() ? Sampler.minFilter.Get() : gltf::MinFilterMode::MinFilter_LINEAR;
				switch (MinFilter)
				{
					case gltf::MinFilterMode::MinFilter_LINEAR:
					case gltf::MinFilterMode::MinFilter_LINEAR_MIPMAP_LINEAR:
						SamplerDesc.emplace_back(CStrID("Filter"), std::string(IsMagPoint ? "min_linear_mag_point_mip_linear" : "linear"));
						break;
					case gltf::MinFilterMode::MinFilter_LINEAR_MIPMAP_NEAREST:
						SamplerDesc.emplace_back(CStrID("Filter"), std::string(IsMagPoint ? "min_linear_magmip_point" : "minmag_linear_mip_point"));
						break;
					case gltf::MinFilterMode::MinFilter_NEAREST:
					case gltf::MinFilterMode::MinFilter_NEAREST_MIPMAP_NEAREST:
						SamplerDesc.emplace_back(CStrID("Filter"), std::string(IsMagPoint ? "point" : "min_point_mag_linear_mip_point"));
						break;
					case gltf::MinFilterMode::MinFilter_NEAREST_MIPMAP_LINEAR:
						SamplerDesc.emplace_back(CStrID("Filter"), std::string(IsMagPoint ? "minmag_point_mip_linear" : "min_point_magmip_linear"));
						break;
				}

				MtlParams.emplace_back(PBRTextureSamplerID, std::move(SamplerDesc));
			}
		}

		// Write resulting file

		const auto MtlRsrcName = GetValidResourceName(Mtl.name);
		const auto DestPath = Ctx.MaterialPath / (MtlRsrcName + ".mtl");

		fs::create_directories(DestPath.parent_path());

		std::ofstream File(DestPath, std::ios_base::binary | std::ios_base::trunc);

		if (!SaveMaterial(File, EffectIt->second, MtlParamTable, MtlParams, Ctx.Log)) return false;

		// Register exported material in the cache

		OutMaterialID = _ResourceRoot + fs::relative(DestPath, _RootDir).generic_string();
		Ctx.ProcessedMaterials.emplace(MtlName, OutMaterialID);

		return true;
	}

	bool ExportTexture(const gltf::TextureInfo& TexInfo, std::string& OutTextureID, std::set<std::string>& OutGLTFSamplers, CContext& Ctx)
	{
		// TODO: maybe support other UVs later, but for now only UV0 is used with PBR materials in DEM
		assert(TexInfo.texCoord == 0);

		const auto& Texture = Ctx.Doc.textures[TexInfo.textureId];

		if (!Texture.samplerId.empty())
			OutGLTFSamplers.insert(Texture.samplerId);

		auto It = Ctx.ProcessedTextures.find(Texture.imageId);
		if (It != Ctx.ProcessedTextures.cend())
		{
			OutTextureID = It->second;
			return true;
		}

		const auto& Image = Ctx.Doc.images[Texture.imageId];

		Ctx.Log.LogDebug("Texture image " + Image.name);

		if (Image.uri.empty())
		{
			Ctx.Log.LogError("FIXME: embedded glTF textures not supported, IMPLEMENT!");
			return false;
		}

		const auto SrcPath = Ctx.SrcFolder / Image.uri;

		const auto RsrcName = GetValidResourceName(fs::path(Image.uri).filename().string());
		const auto DestPath = Ctx.TexturePath / RsrcName;

		try
		{
			// TODO: copy as is for now, but may need format conversion or N one-channel to 1 multi-channel baking
			fs::create_directories(DestPath.parent_path());
			fs::copy_file(SrcPath, DestPath, fs::copy_options::overwrite_existing);
		}
		catch (fs::filesystem_error& e)
		{
			Ctx.Log.LogError("Error copying " + SrcPath.generic_string() + " to " + DestPath.generic_string() + ":\n" + e.what());
			return false;
		}

		OutTextureID = _ResourceRoot + fs::relative(DestPath, _RootDir).generic_string();
		Ctx.ProcessedTextures.emplace(Texture.imageId, OutTextureID);

		return true;
	}

	bool ExportSkin(const std::string& SkinName, std::string& OutSkinID, CContext& Ctx)
	{
		auto It = Ctx.ProcessedSkins.find(SkinName);
		if (It != Ctx.ProcessedSkins.cend())
		{
			OutSkinID = It->second;
			return true;
		}

		const auto& Skin = Ctx.Doc.skins[SkinName];

		Ctx.Log.LogDebug("Skin " + SkinName + ": " + Skin.name);

		std::vector<CBone> Bones;
		Bones.reserve(Skin.jointIds.size());

		// NB: glTF matrices are column-major
		auto InvBindMatrices = gltf::AnimationUtils::GetInverseBindMatrices(Ctx.Doc, *Ctx.ResourceReader, Skin);
		const float* pInvBindMatrix = InvBindMatrices.data();

		// TODO: use for inv bind matrices recalculation?
		//Skin.skeletonId - skeleton root, or use scene root if not specified (root of joint transforms)

		for (const auto& JointID : Skin.jointIds)
		{
			const auto& Joint = Ctx.Doc.nodes[JointID];

			CBone NewBone;
			NewBone.ID = Joint.name.empty() ? JointID : Joint.name;

			if (const auto* pParent = GetParentNode(Ctx.Doc, JointID))
			{
				const auto It = std::find(Skin.jointIds.cbegin(), Skin.jointIds.cend(), pParent->id);
				if (It != Skin.jointIds.cend())
					NewBone.ParentBoneIndex = static_cast<uint16_t>(std::distance(Skin.jointIds.cbegin(), It));
			}

			// Save matrices row-major for DEM (glTF uses column-major)
			for (int Col = 0; Col < 4; ++Col)
				for (int Row = 0; Row < 4; ++Row)
					NewBone.InvLocalBindPose[Col * 4 + Row] = pInvBindMatrix[Row * 4 + Col];

			pInvBindMatrix += 16;

			// TODO: could calculate optional per-bone AABB. May be also useful for ACL.

			Bones.push_back(std::move(NewBone));
		}

		const auto RsrcName = GetValidResourceName(Skin.name.empty() ? Ctx.TaskName + '_' + SkinName : Skin.name);
		const auto DestPath = Ctx.SkinPath / (RsrcName + ".skn");

		if (!WriteDEMSkin(DestPath, Bones, Ctx.Log)) return false;

		OutSkinID = _ResourceRoot + fs::relative(DestPath, _RootDir).generic_string();
		Ctx.ProcessedSkins.emplace(SkinName, OutSkinID);

		return true;
	}

	bool ExportCamera(const std::string& CameraName, CContext& Ctx, Data::CDataArray& Attributes)
	{
		const auto& Camera = Ctx.Doc.cameras[CameraName];

		Ctx.Log.LogDebug("Camera " + Camera.name);

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
				Ctx.Log.LogWarning("Light " + Light.name + " is of unsupported type, skipped");
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

	bool ExportAnimation(const gltf::Animation& Anim, CContext& Ctx)
	{
		const auto RsrcName = GetValidResourceName(Anim.name.empty() ? Ctx.TaskName + '_' + Anim.id : Anim.name);

		Ctx.Log.LogDebug("Animation " + Anim.id + ": " + RsrcName);

		float MinTime = std::numeric_limits<float>().max();
		float MaxTime = std::numeric_limits<float>().lowest();
		for (const auto& Sampler : Anim.samplers.Elements())
		{
			const auto& Accessor = Ctx.Doc.accessors[Sampler.inputAccessorId];
			if (MinTime > Accessor.min[0]) MinTime = Accessor.min[0];
			if (MaxTime < Accessor.max[0]) MaxTime = Accessor.max[0];
		}

		if (MinTime >= MaxTime)
		{
			Ctx.Log.LogWarning("Animation " + RsrcName + " has zero or negative duration, skipped");
			return true;
		}

		const auto FrameCount = static_cast<uint32_t>(std::ceilf((MaxTime - MinTime) * _AnimSamplingRate));

		// Collect nodes animated by this animation
		// Detect their common root (all animated nodes must be in the same scene)
		// Theoretically there can be many roots, as glTF scene can have many top-level nodes
		// Build ACL skeleton bindings for these roots
		// Process all nodes, animating those with curves and passing static values when no curves are present
		//???does ACL support non-animated nodes in the middle of the skeleton?

		//Anim.channels [ sampler -> node property to animate ]
		//Anim.samplers [ key frames -> curve key values + interpolation mode ]

		std::vector<CSkeletonACLBinding> Skeletons;

		for (CSkeletonACLBinding& Skeleton : Skeletons)
		{
			acl::String ClipName(Ctx.ACLAllocator, RsrcName.c_str());
			acl::AnimationClip Clip(Ctx.ACLAllocator, *Skeleton.Skeleton, FrameCount, _AnimSamplingRate, ClipName);

			// fill clip with raw data

			const auto DestPath = Ctx.AnimPath / (RsrcName + ".anm");
			if (!WriteDEMAnimation(DestPath, Ctx.ACLAllocator, Clip, Ctx.Log)) return false;
		}

		return true;
	}
};

int main(int argc, const char** argv)
{
	CGLTFTool Tool("cf-gltf", "glTF 2.0 to DeusExMachina resource converter", { 0, 1, 0 });
	return Tool.Execute(argc, argv);
}
