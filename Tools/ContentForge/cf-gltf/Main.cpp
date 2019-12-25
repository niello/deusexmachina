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

static void BuildNodePath(const gltf::Document& Doc, const gltf::Node* pNode, std::vector<std::string>& OutPath)
{
	const gltf::Node* pCurrNode = pNode;
	while (pCurrNode)
	{
		OutPath.push_back(pCurrNode->id);
		pCurrNode = GetParentNode(Doc, pCurrNode->id);
	}
}
//---------------------------------------------------------------------

static acl::Transform_32 GetNodeTransform(const gltf::Node& Node)
{
	if (Node.matrix != gltf::Matrix4::IDENTITY)
	{
		const acl::AffineMatrix_32 ACLMatrix = acl::matrix_set(
			acl::Vector4_32{ Node.matrix.values[0], Node.matrix.values[1], Node.matrix.values[2], Node.matrix.values[3] },
			acl::Vector4_32{ Node.matrix.values[4], Node.matrix.values[5], Node.matrix.values[6], Node.matrix.values[7] },
			acl::Vector4_32{ Node.matrix.values[8], Node.matrix.values[9], Node.matrix.values[10], Node.matrix.values[11] },
			acl::Vector4_32{ 0.f, 0.f, 0.f, 1.f });

		return
		{
			acl::quat_from_matrix(acl::matrix_remove_scale(ACLMatrix)),
			{ Node.matrix.values[12], Node.matrix.values[13], Node.matrix.values[14], 1.f },
			{ acl::vector_length3(ACLMatrix.x_axis), acl::vector_length3(ACLMatrix.y_axis), acl::vector_length3(ACLMatrix.z_axis), 0.f }
		};
	}
	else
	{
		return
		{
			{ Node.rotation.x, Node.rotation.y, Node.rotation.z, Node.rotation.w },
			{ Node.translation.x, Node.translation.y, Node.translation.z, 1.f },
			{ Node.scale.x, Node.scale.y, Node.scale.z, 0.f }
		};
	}
}
//---------------------------------------------------------------------

static acl::AffineMatrix_32 GetNodeMatrix(const gltf::Node& Node)
{
	if (Node.matrix != gltf::Matrix4::IDENTITY)
	{
		return acl::matrix_set(
			acl::Vector4_32{ Node.matrix.values[0], Node.matrix.values[1], Node.matrix.values[2], Node.matrix.values[3] },
			acl::Vector4_32{ Node.matrix.values[4], Node.matrix.values[5], Node.matrix.values[6], Node.matrix.values[7] },
			acl::Vector4_32{ Node.matrix.values[8], Node.matrix.values[9], Node.matrix.values[10], Node.matrix.values[11] },
			acl::Vector4_32{ Node.matrix.values[12], Node.matrix.values[13], Node.matrix.values[14], 1.f });
	}
	else
	{
		const acl::Transform_32 Tfm =
		{
			{ Node.rotation.x, Node.rotation.y, Node.rotation.z, Node.rotation.w },
			{ Node.translation.x, Node.translation.y, Node.translation.z, 1.f },
			{ Node.scale.x, Node.scale.y, Node.scale.z, 0.f }
		};

		return acl::matrix_from_transform(Tfm);
	}
}
//---------------------------------------------------------------------

std::vector<float> GetJointWeights32x4(const gltf::Document& doc, const gltf::GLTFResourceReader& reader, const gltf::Accessor& accessor)
{
	if (accessor.type != gltf::TYPE_VEC4)
	{
		throw gltf::GLTFException("Invalid type for weights accessor " + accessor.id);
	}

	switch (accessor.componentType)
	{
		case gltf::COMPONENT_FLOAT:
			return reader.ReadBinaryData<float>(doc, accessor);

		case gltf::COMPONENT_UNSIGNED_BYTE:
		{
			auto raw = reader.ReadBinaryData<uint8_t>(doc, accessor);
			std::vector<float> result(raw.size());
			for (size_t i = 0; i < raw.size(); ++i)
				result[i] = gltf::Math::ByteToFloat(raw[i]);
			return std::move(result);
		}

		case gltf::COMPONENT_UNSIGNED_SHORT:
		{
			auto raw = reader.ReadBinaryData<uint16_t>(doc, accessor);
			std::vector<float> result(raw.size());
			for (size_t i = 0; i < raw.size(); ++i)
				result[i] = ShortToNormalizedFloat(raw[i]);
			return std::move(result);
		}

		default:
			throw gltf::GLTFException("Invalid componentType for weights accessor " + accessor.id);
	}
}
//---------------------------------------------------------------------

std::vector<uint16_t> GetJointWeights16x4(const gltf::Document& doc, const gltf::GLTFResourceReader& reader, const gltf::Accessor& accessor)
{
	if (accessor.type != gltf::TYPE_VEC4)
	{
		throw gltf::GLTFException("Invalid type for weights accessor " + accessor.id);
	}

	switch (accessor.componentType)
	{
		case gltf::COMPONENT_FLOAT:
		{
			auto raw = reader.ReadBinaryData<float>(doc, accessor);
			std::vector<uint16_t> result(raw.size());
			for (size_t i = 0; i < raw.size(); ++i)
				result[i] = NormalizedFloatToShort(raw[i]);
			return std::move(result);
		}

		case gltf::COMPONENT_UNSIGNED_BYTE:
		{
			auto raw = reader.ReadBinaryData<uint8_t>(doc, accessor);
			std::vector<uint16_t> result(raw.size());
			for (size_t i = 0; i < raw.size(); ++i)
				result[i] = NormalizedFloatToShort(gltf::Math::ByteToFloat(raw[i]));
			return std::move(result);
		}

		case gltf::COMPONENT_UNSIGNED_SHORT:
			return reader.ReadBinaryData<uint16_t>(doc, accessor);

		default:
			throw gltf::GLTFException("Invalid componentType for weights accessor " + accessor.id);
	}
}
//---------------------------------------------------------------------

static void BuildACLSkeleton(const std::string& NodeID, uint16_t ParentIndex, const std::multimap<std::string, std::string>& Hierarchy,
	std::vector<acl::RigidBone>& Bones, std::vector<const gltf::Node*>& Nodes, const gltf::Document& Doc)
{
	acl::RigidBone Bone;
	Bone.parent_index = ParentIndex;

	// TODO: metric from per-bone AABBs?
	Bone.vertex_distance = 3.f;

	const auto BoneIndex = static_cast<uint16_t>(Bones.size());

	Nodes.push_back(&Doc.nodes[NodeID]);
	Bones.push_back(std::move(Bone));

	const auto Range = Hierarchy.equal_range(NodeID);
	for (auto It = Range.first; It != Range.second; ++It)
		BuildACLSkeleton(It->second, BoneIndex, Hierarchy, Bones, Nodes, Doc);
}
//---------------------------------------------------------------------

static inline gltf::Vector3 LerpVector3(const gltf::Vector3& a, const gltf::Vector3& b, float Factor)
{
	gltf::Vector3 Result;
	Result.x = a.x * (1.f - Factor) + b.x * Factor;
	Result.y = a.y * (1.f - Factor) + b.y * Factor;
	Result.z = a.z * (1.f - Factor) + b.z * Factor;
	return Result;
}
//---------------------------------------------------------------------

static inline gltf::Quaternion LerpQuaternion(const gltf::Quaternion& a, const gltf::Quaternion& b, float Factor)
{
	// Is slerp?
	const auto q = acl::quat_lerp({ a.x, a.y, a.z, a.w }, { b.x, b.y, b.z, b.w }, Factor);
	return { acl::quat_get_x(q), acl::quat_get_y(q), acl::quat_get_z(q), acl::quat_get_w(q) };
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

		std::string NodeID = Node.name.empty() ? Ctx.TaskName + '_' + Node.id : Node.name;

		Ctx.Log.LogDebug("Node " + NodeID);

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
			std::string RootSearchPath;
			if (!ExportSkin(Node, SkinID, RootSearchPath, Ctx)) return false;

			Data::CParams SkinAttribute;
			SkinAttribute.emplace_back(CStrID("Class"), 'SKIN'); //std::string("Frame::CSkinAttribute"));
			SkinAttribute.emplace_back(CStrID("SkinInfo"), SkinID);
			if (!RootSearchPath.empty())
				SkinAttribute.emplace_back(CStrID("RootSearchPath"), RootSearchPath);
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

		constexpr acl::Vector4_32 Unit3 = { 1.f, 1.f, 1.f, 0.f };
		constexpr acl::Vector4_32 Zero3 = { 0.f, 0.f, 0.f, 0.f };
		constexpr acl::Quat_32 IdentityQuat = { 0.f, 0.f, 0.f, 1.f };

		const auto Tfm = GetNodeTransform(Node);

		if (!acl::vector_all_near_equal3(Tfm.scale, Unit3))
			NodeSection.emplace_back(sidScale, vector4({ acl::vector_get_x(Tfm.scale), acl::vector_get_y(Tfm.scale), acl::vector_get_z(Tfm.scale) }));

		if (!acl::quat_near_equal(Tfm.rotation, IdentityQuat))
			NodeSection.emplace_back(sidRotation, vector4({ acl::quat_get_x(Tfm.rotation), acl::quat_get_y(Tfm.rotation), acl::quat_get_z(Tfm.rotation), acl::quat_get_w(Tfm.rotation) }));

		if (!acl::vector_all_near_equal3(Tfm.translation, Zero3))
			NodeSection.emplace_back(sidTranslation, vector4({ acl::vector_get_x(Tfm.translation), acl::vector_get_y(Tfm.translation), acl::vector_get_z(Tfm.translation) }));

		// Process children

		Data::CParams Children;

		for (const auto& Child : Node.children)
			if (!ExportNode(Child, Ctx, Children)) return false;

		if (!Children.empty())
			NodeSection.emplace_back(sidChildren, std::move(Children));

		if (ParamsUtils::HasParam(Nodes, CStrID(NodeID)))
			Ctx.Log.LogWarning("Duplicated node overwritten with name " + Node.name);

		Nodes.emplace_back(CStrID(NodeID), std::move(NodeSection));

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
			ModelAttribute.emplace_back(CStrID("Class"), 'MDLA'); //std::string("Frame::CModelAttribute"));
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
		VertexFormat.BlendWeightSize = 16;
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
				if (VertexFormat.BlendWeightSize != 8 && VertexFormat.BlendWeightSize != 16 && VertexFormat.BlendWeightSize != 32)
					Ctx.Log.LogWarning("Unsupported blend weight size, defaulting to full-precision floats (32). Supported values are 8/16/32.");

				// Must have joint indices
				assert(VertexFormat.BonesPerVertex == 4);

				constexpr bool RenormalizeWeights = true;

				if (VertexFormat.BlendWeightSize == 8)
				{
					const auto Weights = gltf::MeshPrimitiveUtils::GetJointWeights32(Ctx.Doc, *Ctx.ResourceReader, Ctx.Doc.accessors[AccessorId]);
					assert(Weights.size() == RawVertices.size());

					auto AttrIt = Weights.cbegin();
					for (auto& Vertex : RawVertices)
					{
						if (RenormalizeWeights)
						{
							const uint32_t Packed = *AttrIt++;
							auto W1 = static_cast<uint8_t>((Packed >> 0) & 0xff);
							auto W2 = static_cast<uint8_t>((Packed >> 8) & 0xff);
							auto W3 = static_cast<uint8_t>((Packed >> 16) & 0xff);
							auto W4 = static_cast<uint8_t>((Packed >> 24) & 0xff);

							NormalizeWeights8x4(W1, W2, W3, W4);

							Vertex.BlendWeights8 = (W4 << 24) | (W3 << 16) | (W2 << 8) | W1;
						}
						else
						{
							Vertex.BlendWeights8 = *AttrIt++;
						}
					}
				}
				else if (VertexFormat.BlendWeightSize == 16)
				{
					const auto Weights = GetJointWeights16x4(Ctx.Doc, *Ctx.ResourceReader, Ctx.Doc.accessors[AccessorId]);
					assert(Weights.size() == RawVertices.size() * 4);

					auto AttrIt = Weights.cbegin();
					for (auto& Vertex : RawVertices)
					{
						Vertex.BlendWeights16[0] = *AttrIt++;
						Vertex.BlendWeights16[1] = *AttrIt++;
						Vertex.BlendWeights16[2] = *AttrIt++;
						Vertex.BlendWeights16[3] = *AttrIt++;

						if (RenormalizeWeights)
							NormalizeWeights16x4(Vertex.BlendWeights16[0], Vertex.BlendWeights16[1], Vertex.BlendWeights16[2], Vertex.BlendWeights16[3]);
					}
				}
				else
				{
					const auto Weights = GetJointWeights32x4(Ctx.Doc, *Ctx.ResourceReader, Ctx.Doc.accessors[AccessorId]);
					assert(Weights.size() == RawVertices.size() * 4);

					auto AttrIt = Weights.cbegin();
					for (auto& Vertex : RawVertices)
					{
						Vertex.BlendWeights32[0] = *AttrIt++;
						Vertex.BlendWeights32[1] = *AttrIt++;
						Vertex.BlendWeights32[2] = *AttrIt++;
						Vertex.BlendWeights32[3] = *AttrIt++;

						if (RenormalizeWeights)
							NormalizeWeights32x4(Vertex.BlendWeights32[0], Vertex.BlendWeights32[1], Vertex.BlendWeights32[2], Vertex.BlendWeights32[3]);
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

		if (Image.uri.empty())
		{
			Ctx.Log.LogError("FIXME: embedded glTF textures not supported, IMPLEMENT!");
			return false;
		}

		Ctx.Log.LogDebug("Texture image " + Image.id + ": " + Image.uri + " (" + Image.name + ')');

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

	bool ExportSkin(const gltf::Node& SkinNode, std::string& OutSkinID, std::string& OutRootSearchPath, CContext& Ctx)
	{
		auto It = Ctx.ProcessedSkins.find(SkinNode.skinId);
		if (It != Ctx.ProcessedSkins.cend())
		{
			OutSkinID = It->second;
			return true;
		}

		const auto& Skin = Ctx.Doc.skins[SkinNode.skinId];

		Ctx.Log.LogDebug("Skin " + SkinNode.skinId + ": " + Skin.name);

		// Calculate relative path from the skin node to the root joint

		// Since bind matrices are relative to the skeleton root, we must add
		// a transformation from the mesh node to the skeleton root to each of them.
		auto MeshToRoot = acl::matrix_identity_32();

		if (SkinNode.id != Skin.skeletonId)
		{
			std::vector<std::string> CurrNodePath;
			BuildNodePath(Ctx.Doc, &SkinNode, CurrNodePath);

			std::vector<std::string> SkeletonRootNodePath;
			if (!Skin.skeletonId.empty())
			{
				BuildNodePath(Ctx.Doc, &Ctx.Doc.nodes[Skin.skeletonId], SkeletonRootNodePath);

				while (!CurrNodePath.empty() &&
					!SkeletonRootNodePath.empty() &&
					CurrNodePath.back() == SkeletonRootNodePath.back())
				{
					CurrNodePath.pop_back();
					SkeletonRootNodePath.pop_back();
				}
			}

			for (const auto& NodeID : CurrNodePath)
			{
				if (!OutRootSearchPath.empty()) OutRootSearchPath += '.';
				OutRootSearchPath += '^';

				// Accumulate transform from the mesh node to the common parent
				const auto InvLocalMatrix = acl::matrix_inverse(GetNodeMatrix(Ctx.Doc.nodes[NodeID]));
				MeshToRoot = acl::matrix_mul(MeshToRoot, InvLocalMatrix);
			}

			SkeletonRootNodePath.erase(SkeletonRootNodePath.begin());
			for (auto It = SkeletonRootNodePath.crbegin(); It != SkeletonRootNodePath.crend(); ++It)
			{
				const auto& Node = Ctx.Doc.nodes[*It];

				// When not erasing root node itself:
				//if (It.base() - 1 != SkeletonRootNodePath.cbegin())
				//{
					if (!OutRootSearchPath.empty()) OutRootSearchPath += '.';

					OutRootSearchPath += (Node.name.empty() ? Ctx.TaskName + '_' + Node.id : Node.name);
				//}

				// Accumulate transform from the common parent to the skeleton root

				MeshToRoot = acl::matrix_mul(MeshToRoot, GetNodeMatrix(Node));
			}
		}

		// Collect bone info

		std::vector<CBone> Bones;
		Bones.reserve(Skin.jointIds.size());

		// glTF matrices are column-major, like in DEM
		auto InvBindMatrices = gltf::AnimationUtils::GetInverseBindMatrices(Ctx.Doc, *Ctx.ResourceReader, Skin);
		const float* pInvBindMatrix = InvBindMatrices.data();

		for (const auto& JointID : Skin.jointIds)
		{
			const auto& Joint = Ctx.Doc.nodes[JointID];

			CBone NewBone;
			NewBone.ID = Joint.name.empty() ? Ctx.TaskName + '_' + Joint.id : Joint.name;

			if (const auto* pParent = GetParentNode(Ctx.Doc, JointID))
			{
				const auto It = std::find(Skin.jointIds.cbegin(), Skin.jointIds.cend(), pParent->id);
				if (It != Skin.jointIds.cend())
					NewBone.ParentBoneIndex = static_cast<uint16_t>(std::distance(Skin.jointIds.cbegin(), It));
			}

			//auto InvBindMatrix = acl::matrix_mul(MeshToRoot, acl::unaligned_load<acl::AffineMatrix_32>(pInvBindMatrix));
			auto InvBindMatrix = acl::unaligned_load<acl::AffineMatrix_32>(pInvBindMatrix);

			acl::unaligned_write(InvBindMatrix, NewBone.InvLocalBindPose);

			pInvBindMatrix += 16;

			// TODO: could calculate optional per-bone AABB. May be also useful for ACL.

			Bones.push_back(std::move(NewBone));
		}

		// Write resulting file

		const auto RsrcName = GetValidResourceName(Skin.name.empty() ? Ctx.TaskName + '_' + SkinNode.skinId : Skin.name);
		const auto DestPath = Ctx.SkinPath / (RsrcName + ".skn");

		if (!WriteDEMSkin(DestPath, Bones, Ctx.Log)) return false;

		OutSkinID = _ResourceRoot + fs::relative(DestPath, _RootDir).generic_string();
		Ctx.ProcessedSkins.emplace(SkinNode.skinId, OutSkinID);

		return true;
	}

	bool ExportCamera(const std::string& CameraName, CContext& Ctx, Data::CDataArray& Attributes)
	{
		const auto& Camera = Ctx.Doc.cameras[CameraName];

		Ctx.Log.LogDebug("Camera " + Camera.name);

		Data::CParams Attribute;
		Attribute.emplace_back(CStrID("Class"), 'CAMR'); //std::string("Frame::CCameraAttribute"));

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
		Attribute.emplace_back(CStrID("Class"), 'LGTA'); //std::string("Frame::CLightAttribute"));
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

		// Calculate animation duration in seconds and frames

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

		// Collect animation data for each animated node

		struct CNodeAnimation
		{
			// For verification only
			std::string KeyTimesAccessorID;

			std::vector<float> KeyTimes;
			std::vector<float> ScalingValues;
			std::vector<float> RotationValues;
			std::vector<float> TranslationValues;
		};

		std::set<std::string> OpenList;
		std::map<std::string, CNodeAnimation> AnimatedNodes;
		for (const auto& Channel : Anim.channels.Elements())
		{
			if (Channel.target.path != gltf::TargetPath::TARGET_SCALE &&
				Channel.target.path != gltf::TargetPath::TARGET_ROTATION &&
				Channel.target.path != gltf::TargetPath::TARGET_TRANSLATION)
			{
				continue;
			}

			OpenList.insert(Channel.target.nodeId);

			auto It = AnimatedNodes.find(Channel.target.nodeId);
			if (It == AnimatedNodes.end())
				It = AnimatedNodes.emplace(Channel.target.nodeId, CNodeAnimation{}).first;

			const auto& Sampler = Anim.samplers[Channel.samplerId];

			// Time source must be the same for all SRT channels
			if (It->second.KeyTimes.empty())
			{
				It->second.KeyTimes = std::move(gltf::AnimationUtils::GetKeyframeTimes(Ctx.Doc, *Ctx.ResourceReader, Sampler));
				It->second.KeyTimesAccessorID = Sampler.inputAccessorId;
			}
			else
			{
				assert(It->second.KeyTimesAccessorID == Sampler.inputAccessorId);
			}

			// Only linear interpolation is supported now
			assert(Sampler.interpolation == gltf::InterpolationType::INTERPOLATION_LINEAR);

			switch (Channel.target.path)
			{
				case gltf::TargetPath::TARGET_SCALE:
					It->second.ScalingValues = std::move(gltf::AnimationUtils::GetScales(Ctx.Doc, *Ctx.ResourceReader, Sampler));
					break;
				case gltf::TargetPath::TARGET_ROTATION:
					It->second.RotationValues = std::move(gltf::AnimationUtils::GetRotations(Ctx.Doc, *Ctx.ResourceReader, Sampler));
					break;
				case gltf::TargetPath::TARGET_TRANSLATION:
					It->second.TranslationValues = std::move(gltf::AnimationUtils::GetTranslations(Ctx.Doc, *Ctx.ResourceReader, Sampler));
					break;
			}
		}

		// Build parent-child pairs from the scene root to all animated nodes

		// TODO: build hierarchy once for the whole scene?
		std::multimap<std::string, std::string> Hierarchy;
		std::set<std::string> ClosedList;
		while (!OpenList.empty())
		{
			std::string CurrNodeID = std::move(*OpenList.begin());
			OpenList.erase(OpenList.begin());

			if (auto pParent = GetParentNode(Ctx.Doc, CurrNodeID))
			{
				Hierarchy.emplace(pParent->id, CurrNodeID);
				if (ClosedList.find(pParent->id) == ClosedList.cend())
					OpenList.insert(pParent->id);
			}
			else
			{
				// Insert global root. If animated nodes have no common root, this will be the one.
				Hierarchy.emplace(std::string(), CurrNodeID);
			}

			ClosedList.insert(std::move(CurrNodeID));
		}

		// Find the deepest common root for all animated nodes

		// Start from the global root, not present in glTF and never animated
		std::string RootNodeID;
		while (true)
		{
			const auto Range = Hierarchy.equal_range(RootNodeID);
			const auto ChildCount = std::distance(Range.first, Range.second);
			assert(ChildCount > 0);

			// Node has more than one branch with animated nodes, so it is a root
			if (ChildCount > 1) break;

			RootNodeID = Range.first->second;

			// Node's only child is animated, this child is a root
			if (AnimatedNodes.find(RootNodeID) != AnimatedNodes.cend()) break;
		}

		// Build ACL skeleton from hierarchy data

		std::vector<acl::RigidBone> Bones;
		std::vector<const gltf::Node*> Nodes;
		BuildACLSkeleton(RootNodeID, acl::k_invalid_bone_index, Hierarchy, Bones, Nodes, Ctx.Doc);

		assert(Bones.size() <= std::numeric_limits<uint16_t>().max());

		std::vector<std::string> NodeNames(Nodes.size());
		for (size_t BoneIdx = 0; BoneIdx < Nodes.size(); ++BoneIdx)
			NodeNames[BoneIdx] = (Nodes[BoneIdx]->name.empty() ? Ctx.TaskName + '_' + Nodes[BoneIdx]->id : Nodes[BoneIdx]->name);

		acl::RigidSkeletonPtr Skeleton = acl::make_unique<acl::RigidSkeleton>(
			Ctx.ACLAllocator, Ctx.ACLAllocator, Bones.data(), static_cast<uint16_t>(Bones.size()));

		acl::String ClipName(Ctx.ACLAllocator, RsrcName.c_str());
		acl::AnimationClip Clip(Ctx.ACLAllocator, *Skeleton, FrameCount + 1, _AnimSamplingRate, ClipName);

		// Sample animation curves and write into ACL

		CNodeAnimation NoAnimation;

		for (uint32_t SampleIndex = 0; SampleIndex <= FrameCount; ++SampleIndex)
		{
			const float Time = MinTime + static_cast<float>(SampleIndex) / _AnimSamplingRate;

			for (size_t BoneIdx = 0; BoneIdx < Nodes.size(); ++BoneIdx)
			{
				const auto pNode = Nodes[BoneIdx];

				// Initialize with static node transform by default
				//???or can ignore completely in ACL if channel is not animated? Would be especially good for completely not animated nodes!
				auto Scaling = pNode->scale;
				auto Rotation = pNode->rotation;
				auto Translation = pNode->translation;

				const auto It = AnimatedNodes.find(pNode->id);
				if (It != AnimatedNodes.cend() && !It->second.KeyTimes.empty())
				{
					const CNodeAnimation& NodeAnim = It->second;

					auto pScalings = reinterpret_cast<const gltf::Vector3*>(NodeAnim.ScalingValues.data());
					auto pRotations = reinterpret_cast<const gltf::Quaternion*>(NodeAnim.RotationValues.data());
					auto pTranslations = reinterpret_cast<const gltf::Vector3*>(NodeAnim.TranslationValues.data());

					constexpr float Tolerance = 0.0001f;

					auto ItNextKey = std::lower_bound(NodeAnim.KeyTimes.cbegin(), NodeAnim.KeyTimes.cend(), Time - Tolerance);
					const auto NextKeyNumber = std::distance(NodeAnim.KeyTimes.cbegin(), ItNextKey);
					const bool IsLastKey = (ItNextKey == NodeAnim.KeyTimes.cend());
					if (NextKeyNumber == 0 || IsLastKey || CompareFloat(*ItNextKey, Time, Tolerance))
					{
						// Get keyframe value without interpolation
						const auto KeyNumber = IsLastKey ? NextKeyNumber - 1 : NextKeyNumber;
						Scaling = pScalings[KeyNumber];
						Rotation = pRotations[KeyNumber];
						Translation = pTranslations[KeyNumber];
					}
					else
					{
						// Interpolate
						auto ItPrevKey = ItNextKey;
						--ItPrevKey;
						const float PrevTime = *ItPrevKey;
						const float Factor = (Time - PrevTime) / (*ItNextKey - PrevTime);

						Scaling = LerpVector3(pScalings[NextKeyNumber - 1], pScalings[NextKeyNumber], Factor);
						Rotation = LerpQuaternion(pRotations[NextKeyNumber - 1], pRotations[NextKeyNumber], Factor);
						Translation = LerpVector3(pTranslations[NextKeyNumber - 1], pTranslations[NextKeyNumber], Factor);
					}
				}

				acl::AnimatedBone& Bone = Clip.get_animated_bone(static_cast<uint16_t>(BoneIdx));
				Bone.scale_track.set_sample(SampleIndex, { Scaling.x, Scaling.y, Scaling.z, 1.0 });
				Bone.rotation_track.set_sample(SampleIndex, { Rotation.x, Rotation.y, Rotation.z, Rotation.w });
				Bone.translation_track.set_sample(SampleIndex, { Translation.x, Translation.y, Translation.z, 1.0 });
			}
		}

		const auto DestPath = Ctx.AnimPath / (RsrcName + ".anm");
		return WriteDEMAnimation(DestPath, Ctx.ACLAllocator, Clip, NodeNames, Ctx.Log);
	}
};

int main(int argc, const char** argv)
{
	CGLTFTool Tool("cf-gltf", "glTF 2.0 to DeusExMachina resource converter", { 1, 0, 0 });
	return Tool.Execute(argc, argv);
}
