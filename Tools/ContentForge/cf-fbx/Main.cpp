#include <ContentForgeTool.h>
#include <Render/RenderEnums.h>
#include <Utils.h>
#include <ParamsUtils.h>
#include <CLI11.hpp>
#include <fbxsdk.h>
#include <meshoptimizer.h>
#include <acl/core/ansi_allocator.h>
#include <acl/core/unique_ptr.h>
#include <acl/algorithm/uniformly_sampled/encoder.h>

// TODO: look at fbx2acl
namespace acl
{
	typedef std::unique_ptr<AnimationClip, Deleter<AnimationClip>> AnimationClipPtr;
	typedef std::unique_ptr<RigidSkeleton, Deleter<RigidSkeleton>> RigidSkeletonPtr;
}

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/scenes

constexpr size_t MaxUV = 4;
constexpr size_t MaxBonesPerVertex = 4;

struct float2
{
	union
	{
		float v[2];
		struct
		{
			float x, y;
		};
	};

	float2() : x(0.f), y(0.f) {}
	float2(const FbxDouble2& Value)
		: x(static_cast<float>(Value[0]))
		, y(static_cast<float>(Value[1]))
	{}

	float2 operator =(const FbxDouble2& Value)
	{
		x = static_cast<float>(Value[0]);
		y = static_cast<float>(Value[1]);
		return *this;
	}
};

struct float3
{
	union
	{
		float v[3];
		struct
		{
			float x, y, z;
		};
	};

	float3() : x(0.f), y(0.f), z(0.f) {}
	float3(const FbxDouble3& Value)
		: x(static_cast<float>(Value[0]))
		, y(static_cast<float>(Value[1]))
		, z(static_cast<float>(Value[2]))
	{}

	float3 operator =(const FbxDouble3& Value)
	{
		x = static_cast<float>(Value[0]);
		y = static_cast<float>(Value[1]);
		z = static_cast<float>(Value[2]);
		return *this;
	}
};

struct float4
{
	union
	{
		float v[4];
		struct
		{
			float x, y, z, w;
		};
	};

	float4() : x(0.f), y(0.f), z(0.f), w(0.f) {}

	float4(const FbxDouble4& Value)
		: x(static_cast<float>(Value[0]))
		, y(static_cast<float>(Value[1]))
		, z(static_cast<float>(Value[2]))
		, w(static_cast<float>(Value[3]))
	{}

	float4(const FbxColor& Value)
		: x(static_cast<float>(Value[0]))
		, y(static_cast<float>(Value[1]))
		, z(static_cast<float>(Value[2]))
		, w(static_cast<float>(Value[3]))
	{}

	float4 operator =(const FbxDouble4& Value)
	{
		x = static_cast<float>(Value[0]);
		y = static_cast<float>(Value[1]);
		z = static_cast<float>(Value[2]);
		w = static_cast<float>(Value[3]);
		return *this;
	}

	float4 operator =(const FbxColor& Value)
	{
		x = static_cast<float>(Value[0]);
		y = static_cast<float>(Value[1]);
		z = static_cast<float>(Value[2]);
		w = static_cast<float>(Value[3]);
		return *this;
	}
};

// TODO: remove if not required
static void SetupDestinationSRTRecursive(FbxNode* pNode)
{
	FbxVector4 lZero(0, 0, 0);

	// Activate pivot converting
	pNode->SetPivotState(FbxNode::eSourcePivot, FbxNode::ePivotActive);
	pNode->SetPivotState(FbxNode::eDestinationPivot, FbxNode::ePivotActive);

	// We want to set all these to 0 and bake them into the transforms.
	pNode->SetPostRotation(FbxNode::eDestinationPivot, lZero);
	pNode->SetPreRotation(FbxNode::eDestinationPivot, lZero);
	pNode->SetRotationOffset(FbxNode::eDestinationPivot, lZero);
	pNode->SetScalingOffset(FbxNode::eDestinationPivot, lZero);
	pNode->SetRotationPivot(FbxNode::eDestinationPivot, lZero);
	pNode->SetScalingPivot(FbxNode::eDestinationPivot, lZero);

	// Bake rotation order
	pNode->SetRotationOrder(FbxNode::eDestinationPivot, FbxEuler::eOrderXYZ);

	// Bake geometric transforms to avoid creating DEM nodes for them
	pNode->SetGeometricTranslation(FbxNode::eDestinationPivot, lZero);
	pNode->SetGeometricRotation(FbxNode::eDestinationPivot, lZero);
	pNode->SetGeometricScaling(FbxNode::eDestinationPivot, lZero);

	// DEM is capable of slerp, but this setting probably makes sense inside the FBX SDK, not with resulting anim samples
	//pNode->SetQuaternionInterpolation(FbxNode::eDestinationPivot, EFbxQuatInterpMode::eQuatInterpSlerp);
	pNode->SetQuaternionInterpolation(FbxNode::eDestinationPivot, pNode->GetQuaternionInterpolation(FbxNode::eSourcePivot));

	for (int i = 0; i < pNode->GetChildCount(); ++i)
		SetupDestinationSRTRecursive(pNode->GetChild(i));
}
//---------------------------------------------------------------------

template<typename TOut, typename TElement>
static void GetVertexElement(TOut& OutValue, TElement* pElement, int ControlPointIndex, int VertexIndex)
{
	if (!pElement) return;

	int ID;
	switch (pElement->GetMappingMode())
	{
		case FbxLayerElement::eByControlPoint: ID = ControlPointIndex; break;
		case FbxLayerElement::eByPolygonVertex: ID = VertexIndex; break;
		case FbxLayerElement::eByPolygon: ID = VertexIndex / 3; break;
		case FbxLayerElement::eAllSame: ID = 0; break;
		default: return;
	}

	if (pElement->GetReferenceMode() != FbxLayerElement::eDirect)
		ID = pElement->GetIndexArray().GetAt(ID);

	OutValue = pElement->GetDirectArray().GetAt(ID);
}
//---------------------------------------------------------------------

static void WriteVertexComponent(std::ostream& Stream, EVertexComponentSemantic Semantic, EVertexComponentFormat Format, uint8_t Index, uint8_t StreamIndex)
{
	WriteStream(Stream, static_cast<uint8_t>(Semantic));
	WriteStream(Stream, static_cast<uint8_t>(Format));
	WriteStream(Stream, Index);
	WriteStream(Stream, StreamIndex);
}
//---------------------------------------------------------------------

class CFBXTool : public CContentForgeTool
{
protected:

	struct CMeshAttrInfo
	{
		std::string MeshID;
		std::vector<const FbxSurfaceMaterial*> Materials; // Per group (submesh)
	};

	struct CContext
	{
		CThreadSafeLog&           Log;

		acl::ANSIAllocator        ACLAllocator;
		acl::CompressionSettings  ACLSettings;

		fs::path                  MeshPath;
		fs::path                  SkinPath;
		fs::path                  AnimPath;
		std::string               DefaultName;
		Data::CParamsSorted       MaterialMap;

		std::unordered_map<const FbxMesh*, CMeshAttrInfo> ProcessedMeshes;
		std::unordered_map<const FbxMesh*, std::string> ProcessedSkins;
	};

	struct CVertex
	{
		int    ControlPointIndex;
		float3 Position;
		float3 Normal;
		float3 Tangent;
		float3 Bitangent;
		float4 Color;
		float2 UV[MaxUV];
		int    BlendIndices[MaxBonesPerVertex];
		float  BlendWeights[MaxBonesPerVertex];
		size_t BonesUsed = 0;
	};

	struct CMeshData
	{
		std::vector<CVertex> Vertices;
		std::vector<unsigned int> Indices;
	};

	struct CBone
	{
		const FbxNode* pBone;
		FbxAMatrix     InvLocalBindPose;
		std::string    ID;
		uint16_t       ParentBoneIndex;
		// TODO: bone object-space or local-space AABB
	};

	struct CSkeletonACLBinding
	{
		acl::RigidSkeletonPtr Skeleton;
		std::vector<FbxNode*> FbxBones; // Indices in this array are used as bone indices in ACL
	};

	FbxManager*                pFBXManager = nullptr;
	Data::CSchemeSet          _SceneSchemes;
	acl::TransformErrorMetric _ACLErrorMetric; // Stateless and therefore reusable

	std::string               _ResourceRoot;
	std::string               _SchemeFile;
	double                    _AnimSamplingRate = 30.0;
	bool                      _OutputBin = false;
	bool                      _OutputHRD = false; // For debug purposes, saves scene hierarchies in a human-readable format

public:

	CFBXTool(const std::string& Name, const std::string& Desc, CVersion Version)
		: CContentForgeTool(Name, Desc, Version)
		, _ResourceRoot("Data:")
	{
		// Set default before parsing command line
		_RootDir = "../../../content";
		_SchemeFile = "../schemes/scene.dss";
	}

	virtual bool SupportsMultithreading() const override
	{
		// FBX SDK as of 2020.0 is not guaranteed to be thread-safe
		return false;
	}

	virtual int Init() override
	{
		pFBXManager = FbxManager::Create();
		if (!pFBXManager) return 1;

		FbxIOSettings* pIOSettings = FbxIOSettings::Create(pFBXManager, IOSROOT);
		pFBXManager->SetIOSettings(pIOSettings);

		if (_ResourceRoot.empty())
			if (_LogVerbosity >= EVerbosity::Warnings)
				std::cout << "Resource root is empty, external references may not be resolved from the game!";

		if (!_OutputHRD) _OutputBin = true;

		if (_OutputBin)
		{
			if (!ParamsUtils::LoadSchemes(_SchemeFile.c_str(), _SceneSchemes)) return 2;
		}

		return 0;
	}

	virtual int Term() override
	{
		pFBXManager->Destroy();
		pFBXManager = nullptr;
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
		// TODO: check whether the metafile can be processed by this tool

		// Import FBX scene from the source file

		FbxImporter* pImporter = FbxImporter::Create(pFBXManager, "");

		const auto SrcPath = Task.SrcFilePath.string();

		if (!pImporter->Initialize(SrcPath.c_str(), -1, pFBXManager->GetIOSettings()))
		{
			Task.Log.LogError("Failed to create FbxImporter for " + SrcPath + ": " + pImporter->GetStatus().GetErrorString());
			pImporter->Destroy();
			return false;
		}

		if (pImporter->IsFBX())
		{
			int Major, Minor, Revision;
			pImporter->GetFileVersion(Major, Minor, Revision);
			Task.Log.LogInfo("Source format: FBX v" + std::to_string(Major) + '.' + std::to_string(Minor) + '.' + std::to_string(Revision));
		}

		FbxScene* pScene = FbxScene::Create(pFBXManager, "SourceScene");

		if (!pImporter->Import(pScene))
		{
			Task.Log.LogError("Failed to import " + SrcPath + ": " + pImporter->GetStatus().GetErrorString());
			pImporter->Destroy();
			return false;
		}

		pImporter->Destroy();

		// Convert scene transforms and geometry to a more suitable format

		const auto SceneFrameRate = FbxTime::GetFrameRate(FbxTime::GetGlobalTimeMode());
		if (!FbxEqual(SceneFrameRate, _AnimSamplingRate))
		{
			FbxTime::SetGlobalTimeMode(FbxTime::eCustom, _AnimSamplingRate);
			Task.Log.LogInfo("Scene frame rate changed from " + std::to_string(static_cast<uint32_t>(SceneFrameRate)) +
				" to " + std::to_string(static_cast<uint32_t>(_AnimSamplingRate)));
		}

		//SetupDestinationSRTRecursive(pScene->GetRootNode());
		//pScene->GetRootNode()->ConvertPivotAnimationRecursive(nullptr, FbxNode::eDestinationPivot, _AnimSamplingRate);

		{
			FbxGeometryConverter GeometryConverter(pFBXManager);

			if (!GeometryConverter.Triangulate(pScene, true))
				Task.Log.LogWarning("Couldn't triangulate some geometry");

			GeometryConverter.RemoveBadPolygonsFromMeshes(pScene);

			// Don't use GeometryConverter.SplitMeshesPerMaterial to preserve integrity of skins
		}

		// Prepare task context

		//!!!TODO: need flags, what to export! command-line override must be provided along with .meta params
		CContext Ctx{ Task.Log };

		Ctx.MaterialMap = ParamsUtils::OrderedParamsToSorted(ParamsUtils::GetParam<Data::CParams>(Task.Params, "Materials", {}));

		Ctx.DefaultName = Task.TaskID.CStr();

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

		// Export node hierarchy to DEM format, omit FBX root node
		
		Data::CParams Nodes;

		for (int i = 0; i < pScene->GetRootNode()->GetChildCount(); ++i)
			if (!ExportNode(pScene->GetRootNode()->GetChild(i), Ctx, Nodes)) return false;

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

		// Each stack is a clip. Only one layer (or combined result of all stack layers) is considered, no
		// blending info saved. Nodes are processed from the root recursively and each can have up to 3 tracks.
		const int AnimationCount = pScene->GetSrcObjectCount<FbxAnimStack>();
		if (AnimationCount)
		{
			for (int i = 0; i < AnimationCount; ++i)
			{
				const auto pAnimStack = static_cast<FbxAnimStack*>(pScene->GetSrcObject<FbxAnimStack>(i));
				if (!ExportAnimation(pAnimStack, pScene, Ctx)) return false;
			}
		}

		// Export additional info

		// pScene->GetGlobalSettings().GetAmbientColor();
		// Scene->GetPoseCount();

		// Finalize and save the scene

		const std::string TaskName = Task.TaskID.ToString();

		Data::CParams Result;
		//if (Nodes.size() == 1)
		//{
		//	// TODO: FBX doesn't have useful data at the root node. Is it a good idea to save its only
		//	// child as a root instead?
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

	bool ExportNode(FbxNode* pNode, CContext& Ctx, Data::CParams& Nodes)
	{
		if (!pNode)
		{
			Ctx.Log.LogWarning("Nullptr FBX node encountered");
			return true;
		}

		Ctx.Log.LogDebug(std::string("Node ") + pNode->GetName());

		static const CStrID sidTranslation("Translation");
		static const CStrID sidRotation("Rotation");
		static const CStrID sidScale("Scale");
		static const CStrID sidAttrs("Attrs");
		static const CStrID sidChildren("Children");

		Data::CParams NodeSection;

		// Process attributes

		Data::CDataArray Attributes;
		bool IsBone = false;

		for (int i = 0; i < pNode->GetNodeAttributeCount(); ++i)
		{
			auto pAttribute = pNode->GetNodeAttributeByIndex(i);

			switch (pAttribute->GetAttributeType())
			{
				case FbxNodeAttribute::eMesh:
				{
					if (!ExportModel(static_cast<FbxMesh*>(pAttribute), Ctx, Attributes)) return false;
					break;
				}
				case FbxNodeAttribute::eLight:
				{
					if (!ExportLight(static_cast<FbxLight*>(pAttribute), Ctx, Attributes)) return false;
					break;
				}
				case FbxNodeAttribute::eCamera:
				{
					if (!ExportCamera(static_cast<FbxCamera*>(pAttribute), Ctx, Attributes)) return false;
					break;
				}
				case FbxNodeAttribute::eLODGroup:
				{
					if (!ExportLODGroup(static_cast<FbxLODGroup*>(pAttribute), Ctx, Attributes)) return false;
					break;
				}
				case FbxNodeAttribute::eSkeleton:
				{
					IsBone = true;
					break;
				}
				case FbxNodeAttribute::eMarker:
				{
					// TODO: can extract some useful data?
					break;
				}
				case FbxNodeAttribute::eNull: break;
				/*
				eShape,
				eCachedEffect,
				eLine
				*/
				default:
				{
					Ctx.Log.LogInfo("Skipped unsupported attribute of type " + std::to_string(pAttribute->GetAttributeType()));
					break;
				}
			}
		}

		if (!Attributes.empty())
			NodeSection.emplace_back(sidAttrs, std::move(Attributes));

		// Process transform

		// Bone transformation is determined by the bind pose and animations
		if (!IsBone)
		{
			float3 Translation;
			float3 Scaling;
			float4 Rotation;

			// Raw properties are left for testing purposes
			constexpr bool UseRawProperties = false;
			if (UseRawProperties)
			{
				Translation = pNode->LclTranslation.Get();
				Scaling = pNode->LclScaling.Get();

				FbxQuaternion Quat;
				Quat.ComposeSphericalXYZ(pNode->LclRotation.Get());
				Rotation = Quat;
			}
			else
			{
				const auto LocalTfm = pNode->EvaluateLocalTransform();
				Translation = LocalTfm.GetT();
				Scaling = LocalTfm.GetS();
				Rotation = LocalTfm.GetQ();
			}

			NodeSection.emplace_back(sidTranslation, vector4(Translation.v, 3));
			NodeSection.emplace_back(sidRotation, vector4(Rotation.v, 4));
			NodeSection.emplace_back(sidScale, vector4(Scaling.v, 3));
		}

		// Process children

		Data::CParams Children;

		for (int i = 0; i < pNode->GetChildCount(); ++i)
		{
			// Blender FBX exporter bug
			if (pNode->GetChild(i)->GetParent() != pNode)
			{
				Ctx.Log.LogWarning("Node " + std::string(pNode->GetName()) + " has a child with broken parent link");
				continue;
			}

			if (!ExportNode(pNode->GetChild(i), Ctx, Children)) return false;
		}

		if (!Children.empty())
			NodeSection.emplace_back(sidChildren, std::move(Children));

		if (ParamsUtils::HasParam(Nodes, CStrID(pNode->GetName())))
			Ctx.Log.LogWarning("Duplicated node overwritten with name " + std::string(pNode->GetName()));

		Nodes.emplace_back(CStrID(pNode->GetName()), std::move(NodeSection));

		return true;
	}

	bool ExportModel(const FbxMesh* pMesh, CContext& Ctx, Data::CDataArray& Attributes)
	{
		Ctx.Log.LogDebug(std::string("Model ") + pMesh->GetName());

		// Export mesh (optionally skinned)

		auto MeshIt = Ctx.ProcessedMeshes.find(pMesh);
		if (MeshIt == Ctx.ProcessedMeshes.cend())
		{
			if (!ExportMesh(pMesh, Ctx)) return false;
			MeshIt = Ctx.ProcessedMeshes.find(pMesh);
			if (MeshIt == Ctx.ProcessedMeshes.cend()) return false;
		}

		const CMeshAttrInfo& MeshInfo = MeshIt->second;

		// Add models per mesh group

		int GroupIndex = 0;
		for (const FbxSurfaceMaterial* pMaterial : MeshInfo.Materials)
		{
			// Export material for the current submesh

			// PBR materials aren't supported in FBX, can't export.
			// Blender FBX exporter saves no textures, so parsing is completely useless for now.
			// Instead of this, materials must be precreated and explicitly declared in .meta.
			// glTF 2.0 exporter will address this issue.

			std::string MaterialID;
			if (pMaterial)
			{
				std::string MaterialName = pMaterial->GetName();
				std::replace(MaterialName.begin(), MaterialName.end(), ' ', '_');

				auto MtlIt = Ctx.MaterialMap.find(CStrID(MaterialName.c_str()));
				if (MtlIt != Ctx.MaterialMap.cend())
				{
					fs::path MtlPath = MtlIt->second.GetValue<std::string>();
					if (!_RootDir.empty() && MtlPath.is_relative())
						MtlPath = fs::path(_RootDir) / MtlPath;

					MaterialID = _ResourceRoot + fs::relative(MtlPath, _RootDir).generic_string();
				}
				else
				{
					Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + ':' + std::to_string(GroupIndex) + " references undefined material " + MaterialID);
				}
			}

			// Assemble a model attribute

			Data::CParams ModelAttribute;
			ModelAttribute.emplace_back(CStrID("Class"), std::string("Frame::CModelAttribute"));
			ModelAttribute.emplace_back(CStrID("Mesh"), MeshInfo.MeshID);
			ModelAttribute.emplace_back(CStrID("MeshGroupIndex"), GroupIndex);
			if (!MaterialID.empty())
				ModelAttribute.emplace_back(CStrID("Material"), MaterialID);
			else
				Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " has a group with no material attached");
			Attributes.push_back(std::move(ModelAttribute));

			++GroupIndex;
		}

		// Assemble the skin attribute if required

		auto SkinIt = Ctx.ProcessedSkins.find(pMesh);
		std::string SkinID = (SkinIt != Ctx.ProcessedSkins.cend()) ? SkinIt->second : std::string{};

		if (!SkinID.empty())
		{
			Data::CParams SkinAttribute;
			SkinAttribute.emplace_back(CStrID("Class"), std::string("Frame::CSkinAttribute"));
			SkinAttribute.emplace_back(CStrID("SkinInfo"), SkinID);
			//SkinAttribute.emplace(CStrID("AutocreateBones"), true);
			Attributes.push_back(std::move(SkinAttribute));
		}

		return true;
	}

	// Export mesh and optional skin to DEM-native format
	bool ExportMesh(const FbxMesh* pMesh, CContext& Ctx)
	{
		// Determine vertex format

		const FbxVector4* pControlPoints = pMesh->GetControlPoints();

		const auto NormalCount = std::min(1, pMesh->GetElementNormalCount());
		if (pMesh->GetElementNormalCount() > NormalCount)
			Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " will use only " + std::to_string(NormalCount) + '/' + std::to_string(pMesh->GetElementNormalCount()) + " normals");

		const auto TangentCount = std::min(1, pMesh->GetElementTangentCount());
		if (pMesh->GetElementTangentCount() > TangentCount)
			Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " will use only " + std::to_string(TangentCount) + '/' + std::to_string(pMesh->GetElementTangentCount()) + " tangents");

		const auto BitangentCount = std::min(1, pMesh->GetElementBinormalCount());
		if (pMesh->GetElementBinormalCount() > BitangentCount)
			Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " will use only " + std::to_string(BitangentCount) + '/' + std::to_string(pMesh->GetElementBinormalCount()) + " bitangents");

		const auto ColorCount = std::min(1, pMesh->GetElementVertexColorCount());
		if (pMesh->GetElementVertexColorCount() > ColorCount)
			Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " will use only " + std::to_string(ColorCount) + '/' + std::to_string(pMesh->GetElementVertexColorCount()) + " colors");

		const auto UVCount = std::min(static_cast<int>(MaxUV), pMesh->GetElementUVCount());
		if (pMesh->GetElementUVCount() > UVCount)
			Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " will use only " + std::to_string(UVCount) + '/' + std::to_string(pMesh->GetElementUVCount()) + " UVs");

		const auto MaterialLayerCount = std::min(1, pMesh->GetElementMaterialCount());
		if (pMesh->GetElementMaterialCount() > MaterialLayerCount)
			Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " will use only " + std::to_string(MaterialLayerCount) + '/' + std::to_string(pMesh->GetElementMaterialCount()) + " material layers");

		// Collect vertices for each material separately

		const int PolyCount = pMesh->GetPolygonCount();
		const auto pMaterialElement = pMesh->GetElementMaterial();

		std::map<const FbxSurfaceMaterial*, CMeshData> SubMeshes;

		for (int p = 0; p < PolyCount; ++p)
		{
			// Polygon must be a triangle
			const int PolySize = pMesh->GetPolygonSize(p);
			if (PolySize > 3)
			{
				Ctx.Log.LogError("Polygon " + std::to_string(p) + " in mesh " + pMesh->GetName() + " is not triangulated, can't proceed");
				return false;
			}
			else if (PolySize < 3)
			{
				Ctx.Log.LogWarning("Degenerate polygon " + std::to_string(p) + " skipped in mesh " + pMesh->GetName());
				continue;
			}

			// Get material index and corresponding group (submesh)

			const FbxSurfaceMaterial* pMaterial = nullptr;
			if (pMaterialElement)
			{
				int MaterialIndex = -1;
				switch (pMaterialElement->GetMappingMode())
				{
					case FbxLayerElement::eByPolygon: MaterialIndex = pMaterialElement->GetIndexArray().GetAt(p); break;
					case FbxLayerElement::eAllSame: MaterialIndex = pMaterialElement->GetIndexArray().GetAt(0); break;
					default: assert(false && "Unsupported material mapping mode");
				}

				pMaterial = pMesh->GetNode()->GetMaterial(MaterialIndex);
			}

			auto GroupIt = SubMeshes.find(pMaterial);
			if (GroupIt == SubMeshes.cend())
			{
				GroupIt = SubMeshes.emplace(pMaterial, CMeshData{}).first;
				GroupIt->second.Vertices.reserve(static_cast<size_t>((PolyCount - p) * 3));
			}

			auto& Vertices = GroupIt->second.Vertices;

			// Process polygon vertices

			for (int v = 0; v < PolySize; ++v)
			{
				const auto VertexIndex = static_cast<unsigned int>(Vertices.size());
				const auto ControlPointIndex = pMesh->GetPolygonVertex(p, v);

				const auto ControlPoint = pControlPoints[ControlPointIndex];

				CVertex Vertex{ 0 };
				Vertex.ControlPointIndex = ControlPointIndex;

				// NB: we need float3 positions for meshopt_optimizeOverdraw
				Vertex.Position = pControlPoints[ControlPointIndex];

				if (NormalCount)
					GetVertexElement(Vertex.Normal, pMesh->GetElementNormal(), ControlPointIndex, VertexIndex);

				if (TangentCount)
					GetVertexElement(Vertex.Tangent, pMesh->GetElementTangent(), ControlPointIndex, VertexIndex);

				if (BitangentCount)
					GetVertexElement(Vertex.Bitangent, pMesh->GetElementBinormal(), ControlPointIndex, VertexIndex);

				if (ColorCount)
					GetVertexElement(Vertex.Color, pMesh->GetElementVertexColor(), ControlPointIndex, VertexIndex);

				for (int e = 0; e < MaxUV; ++e)
					GetVertexElement(Vertex.UV[e], pMesh->GetElementUV(e), ControlPointIndex, VertexIndex);

				Vertices.push_back(std::move(Vertex));
			}
		}

		// Index and optimize vertices

		size_t TotalVertices = 0;
		size_t TotalIndices = 0;
		for (auto& Pair : SubMeshes)
		{
			std::vector<CVertex> RawVertices;
			std::swap(RawVertices, Pair.second.Vertices);

			auto& Indices = Pair.second.Indices;
			Indices.resize(RawVertices.size());
			const auto VertexCount = meshopt_generateVertexRemap(Indices.data(), nullptr, RawVertices.size(), RawVertices.data(), RawVertices.size(), sizeof(CVertex));

			auto& Vertices = Pair.second.Vertices;
			Vertices.resize(VertexCount);
			meshopt_remapVertexBuffer(Vertices.data(), RawVertices.data(), RawVertices.size(), sizeof(CVertex), Indices.data());

			// NB: meshopt_remapIndexBuffer is not needed, as we have no source indices,
			//     and remap array is effectively an index array
			//std::vector<unsigned int> Indices2(Indices.size());
			//meshopt_remapIndexBuffer(Indices2.data(), nullptr, Indices.size(), Indices.data());

			meshopt_optimizeVertexCache(Indices.data(), Indices.data(), Indices.size(), Vertices.size());

			meshopt_optimizeOverdraw(Indices.data(), Indices.data(), Indices.size(), &Vertices[0].Position.x, Vertices.size(), sizeof(CVertex), 1.05f);

			meshopt_optimizeVertexFetch(Vertices.data(), Indices.data(), Indices.size(), Vertices.data(), Vertices.size(), sizeof(CVertex));

			// TODO: meshopt_generateShadowIndexBuffer for Z prepass and shadow rendering.
			// Also can separate positions from all other data into 2 vertex streams, and use only positions for shadows & Z prepass.

			TotalVertices += Vertices.size();
			TotalIndices += Indices.size();
		}

		// Process skin, one for all submeshes
		// NB: skin is per-control-point, so it is better done after optimizing out redundant vertices

		std::vector<CBone> Bones;
		size_t MaxBonesPerVertexUsed = 0;

		const FbxAMatrix InvMeshWorldMatrix = pMesh->GetNode()->EvaluateGlobalTransform().Inverse();

		const auto SkinCount = pMesh->GetDeformerCount(FbxDeformer::eSkin);
		for (int s = 0; s < SkinCount; ++s)
		{
			const FbxSkin* pSkin = static_cast<FbxSkin*>(pMesh->GetDeformer(s, FbxDeformer::eSkin));
			const auto ClusterCount = pSkin->GetClusterCount();

			constexpr uint16_t NoParent = static_cast<uint16_t>(-1);

			for (int c = 0; c < ClusterCount; ++c)
			{
				const FbxCluster* pCluster = pSkin->GetCluster(c);

				const int IndexCount = pCluster->GetControlPointIndicesCount();
				const int* pIndices = pCluster->GetControlPointIndices();
				const double* pWeights = pCluster->GetControlPointWeights();
				const FbxNode* pBone = pCluster->GetLink();

				if (!pBone || !pIndices || !pWeights || !IndexCount)
				{
					Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " has empty bone " + (pBone ? pBone->GetName() : "<null>"));
					continue;
				}

				// Write blend indices and weights to vertices

				const int BoneIndex = static_cast<int>(Bones.size());

				size_t VerticesAffected = 0;
				for (int i = 0; i < IndexCount; ++i)
				{
					const int ControlPointIndex = pIndices[i];
					const float Weight = static_cast<float>(pWeights[i]);

					// Skip (almost) zero weights
					if (CompareFloat(Weight, 0.f)) continue;

					for (auto& Pair : SubMeshes)
					{
						auto& Vertices = Pair.second.Vertices;
						for (auto& Vertex : Vertices)
						{
							if (Vertex.ControlPointIndex != ControlPointIndex) continue;

							size_t BoneOrderNumber;
							if (Vertex.BonesUsed >= MaxBonesPerVertex)
							{
								Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " control point " + std::to_string(ControlPointIndex) + " reached the limit of bones, the least influencing bone discarded");

								BoneOrderNumber = 0;
								for (size_t bo = 1; bo < MaxBonesPerVertex; ++bo)
								{
									if (Vertex.BlendWeights[bo] < Vertex.BlendWeights[BoneOrderNumber])
										BoneOrderNumber = bo;
								}
							}
							else
							{
								BoneOrderNumber = Vertex.BonesUsed;

								++Vertex.BonesUsed;
								if (Vertex.BonesUsed > MaxBonesPerVertexUsed)
									MaxBonesPerVertexUsed = Vertex.BonesUsed;
							}

							Vertex.BlendIndices[BoneOrderNumber] = BoneIndex;
							Vertex.BlendWeights[BoneOrderNumber] = Weight;

							++VerticesAffected;
						}
					}
				}

				// If bone affects nothing it can be dropped
				if (!VerticesAffected) continue;

				// TODO: could calculate optional per-bone AABB. May be also useful for ACL.

				// Calculate inverse local bind pose and save the bone
				FbxAMatrix WorldBindPose;
				pCluster->GetTransformLinkMatrix(WorldBindPose);
				Bones.push_back(CBone{ pBone, (InvMeshWorldMatrix * WorldBindPose).Inverse(), pBone->GetName(), NoParent });
			}

			// Establish parent-child links and check IDs

			// There are a couple of approaches for saving bones:
			// 1. Save node names, ensure they are unique and search all the scene node tree for them
			// 2. Save node name and parent bone index, or full path if parent is not a bone
			// 3. Hybrid - save node name and parent bone index. If parent is not a bone, search like in 1.
			// Approach 3 is used here.

			std::set<std::string> BoneNames;
			for (auto& Bone : Bones)
			{
				if (BoneNames.find(Bone.ID) != BoneNames.cend())
					Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " bone " + Bone.ID + " occurs more than once, skin may be broken");
				else
					BoneNames.insert(Bone.ID);

				const FbxNode* pParent = Bone.pBone->GetParent();
				const auto It = std::find_if(Bones.cbegin(), Bones.cend(), [pParent](const CBone& b) { return b.pBone == pParent; });
				if (It == Bones.cend())
					Bone.ParentBoneIndex = static_cast<uint16_t>(std::distance(Bones.cbegin(), It));
			}
		}

		// TODO: simplify, quantize and compress if required, see meshoptimizer readme, can simplify for lower LODs

		// Write resulting mesh file

		// TODO: replace forbidden characters (std::transform with replacer callback?)
		std::string MeshName = pMesh->GetName();
		if (MeshName.empty()) MeshName = pMesh->GetNode()->GetName();
		if (MeshName.empty()) MeshName = Ctx.DefaultName; //!!!FIXME: add counter per resource type!
		ToLower(MeshName);

		{
			const auto DestPath = Ctx.MeshPath / (MeshName + ".msh");

			fs::create_directories(Ctx.MeshPath);

			std::ofstream File(DestPath, std::ios_base::binary | std::ios_base::trunc);
			if (!File)
			{
				Ctx.Log.LogError("Error opening an output file " + DestPath.string());
				return false;
			}

			WriteStream<uint32_t>(File, 'MESH');        // Format magic value
			WriteStream<uint32_t>(File, 0x00010000);    // Version 0.1.0.0

			WriteStream(File, static_cast<uint32_t>(SubMeshes.size()));
			WriteStream(File, static_cast<uint32_t>(TotalVertices));
			WriteStream(File, static_cast<uint32_t>(TotalIndices));

			// One index size in bytes
			const bool Indices32 = (TotalVertices > std::numeric_limits<uint16_t>().max());
			if (Indices32)
			{
				Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " has " + std::to_string(TotalVertices) + " vertices and will use 32-bit indices");
				WriteStream<uint8_t>(File, 4);
			}
			else
			{
				WriteStream<uint8_t>(File, 2);
			}

			const uint32_t VertexComponentCount =
				1 + // Position
				NormalCount +
				TangentCount +
				BitangentCount +
				UVCount +
				ColorCount +
				(MaxBonesPerVertexUsed ? 2 : 0); // Blend indices and weights

			WriteStream(File, VertexComponentCount);

			WriteVertexComponent(File, EVertexComponentSemantic::VCSem_Position, EVertexComponentFormat::VCFmt_Float32_3, 0, 0);

			for (int i = 0; i < NormalCount; ++i)
				WriteVertexComponent(File, EVertexComponentSemantic::VCSem_Normal, EVertexComponentFormat::VCFmt_Float32_3, i, 0);

			for (int i = 0; i < TangentCount; ++i)
				WriteVertexComponent(File, EVertexComponentSemantic::VCSem_Tangent, EVertexComponentFormat::VCFmt_Float32_3, i, 0);

			for (int i = 0; i < BitangentCount; ++i)
				WriteVertexComponent(File, EVertexComponentSemantic::VCSem_Bitangent, EVertexComponentFormat::VCFmt_Float32_3, i, 0);

			for (int i = 0; i < ColorCount; ++i)
				WriteVertexComponent(File, EVertexComponentSemantic::VCSem_Color, EVertexComponentFormat::VCFmt_Float32_4, i, 0);

			for (int i = 0; i < UVCount; ++i)
				WriteVertexComponent(File, EVertexComponentSemantic::VCSem_TexCoord, EVertexComponentFormat::VCFmt_Float32_2, i, 0);

			if (MaxBonesPerVertexUsed)
			{
				if (Bones.size() > 256)
					// Could use unsigned, but it is not supported in D3D9. > 32k bones is nonsence anyway.
					WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneIndices, EVertexComponentFormat::VCFmt_SInt16_4, 0, 0);
				else
					WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneIndices, EVertexComponentFormat::VCFmt_UInt8_4, 0, 0);

				// Max 4 bones per vertex are supported, at least for now
				assert(MaxBonesPerVertexUsed < 5);

				if (MaxBonesPerVertexUsed == 1)
					WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneWeights, EVertexComponentFormat::VCFmt_Float32_1, 0, 0);
				else if (MaxBonesPerVertexUsed == 2)
					WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneWeights, EVertexComponentFormat::VCFmt_Float32_2, 0, 0);
				else if (MaxBonesPerVertexUsed == 3)
					WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneWeights, EVertexComponentFormat::VCFmt_Float32_3, 0, 0);
				else
					WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneWeights, EVertexComponentFormat::VCFmt_Float32_4, 0, 0);
			}

			// Save mesh groups (always 1 LOD now, may change later)

			// TODO: test if index offset is needed, each submesh starts from zero now
			for (const auto& Pair : SubMeshes)
			{
				const auto& Indices = Pair.second.Indices;
				const auto& Vertices = Pair.second.Vertices;

				WriteStream<uint32_t>(File, 0);                            // First vertex
				WriteStream(File, static_cast<uint32_t>(Vertices.size())); // Vertex count
				WriteStream<uint32_t>(File, 0);                            // First index
				WriteStream(File, static_cast<uint32_t>(Indices.size()));  // Index count
				WriteStream(File, static_cast<uint8_t>(EPrimitiveTopology::Prim_TriList));

				// TODO: precalculate group AABB!
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

					if (NormalCount) WriteStream(File, Vertex.Normal);
					if (TangentCount) WriteStream(File, Vertex.Tangent);
					if (BitangentCount) WriteStream(File, Vertex.Bitangent);
					if (ColorCount) WriteStream(File, Vertex.Color);

					for (int i = 0; i < UVCount; ++i)
						WriteStream(File, Vertex.UV[i]);

					if (MaxBonesPerVertexUsed)
					{
						// Blend indices are always 4-component
						for (int i = 0; i < 4; ++i)
						{
							const int BoneIndex = (i < MaxBonesPerVertex) ? Vertex.BlendIndices[i] : 0;
							if (Bones.size() > 256)
								WriteStream(File, static_cast<int16_t>(BoneIndex));
							else
								WriteStream(File, static_cast<uint8_t>(BoneIndex));
						}

						File.write(reinterpret_cast<const char*>(Vertex.BlendWeights), MaxBonesPerVertexUsed * sizeof(float));
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

			CMeshAttrInfo MeshInfo;
			MeshInfo.MeshID = _ResourceRoot + fs::relative(DestPath, _RootDir).generic_string();
			for (const auto& Pair : SubMeshes)
				MeshInfo.Materials.push_back(Pair.first);

			Ctx.ProcessedMeshes.emplace(pMesh, std::move(MeshInfo)).first->second;
		}

		// Write resulting skin file (if skinned)

		if (MaxBonesPerVertexUsed)
		{
			const auto DestPath = Ctx.SkinPath / (MeshName + ".skn");

			fs::create_directories(Ctx.SkinPath);

			std::ofstream File(DestPath, std::ios_base::binary | std::ios_base::trunc);
			if (!File)
			{
				Ctx.Log.LogError("Error opening an output file " + DestPath.string());
				return false;
			}

			WriteStream<uint32_t>(File, 'SKIN');        // Format magic value
			WriteStream<uint32_t>(File, 0x00010000);    // Version 0.1.0.0
			WriteStream(File, static_cast<uint32_t>(Bones.size()));
			WriteStream<uint32_t>(File, 0);             // Padding to align matrices offset to 16 bytes boundary

			// Save matrices row-major for DEM (FBX SDK uses column-major)
			for (const auto& Bone : Bones)
				for (int i = 0; i < 4; ++i)
					for (int j = 0; j < 4; ++j)
						WriteStream(File, static_cast<float>(Bone.InvLocalBindPose[i][j]));

			for (const auto& Bone : Bones)
			{
				WriteStream(File, Bone.ParentBoneIndex);
				WriteStream(File, Bone.ID);
			}
				
			std::string SkinID = _ResourceRoot + fs::relative(DestPath, _RootDir).generic_string();

			Ctx.ProcessedSkins.emplace(pMesh, std::move(SkinID));
		}

		return true;
	}

	bool ExportLight(FbxLight* pLight, CContext& Ctx, Data::CDataArray& Attributes)
	{
		Ctx.Log.LogDebug("Light");

		int LightType = 0;
		switch (pLight->LightType.Get())
		{
			case FbxLight::eDirectional: LightType = 0; break;
			case FbxLight::ePoint: LightType = 1; break;
			case FbxLight::eSpot: LightType = 2; break;
			default:
			{
				Ctx.Log.LogWarning(std::string("Light ") + pLight->GetName() + " is of unsupported type, skipped");
				return true;
			}
		}

		const float3 Color = pLight->Color.Get();
		const float Intensity = static_cast<float>(pLight->Intensity.Get() / 100.0);

		float Range = 0.f;
		if (LightType != 0)
		{
			if (pLight->EnableFarAttenuation.Get())
			{
				Range = static_cast<float>(pLight->FarAttenuationEnd.Get());
			}
			else
			{
				// TODO: ensure this algorithm is correct, test on real lights!

				// Proportion of original intensity at which we consider the light decayed to nothing
				constexpr float DecayThreshold = 0.01f;

				const float DecayStart = static_cast<float>(pLight->DecayStart.Get());
				if (CompareFloat(DecayStart, 0.f))
				{
					Ctx.Log.LogWarning(std::string("Light ") + pLight->GetName() + " has infinite range, skipped as unsupported");
					return true;
				}

				const float LinearRange = Intensity / (DecayStart * DecayThreshold);

				switch (pLight->DecayType.Get())
				{
					case FbxLight::eLinear: Range = LinearRange; break;
					case FbxLight::eQuadratic: Range = std::sqrtf(LinearRange); break;
					case FbxLight::eCubic: Range = std::powf(LinearRange, 1.f / 3.f); break;
					default:
					{
						Ctx.Log.LogWarning(std::string("Light ") + pLight->GetName() + " has unsupported decay type, skipped");
						return true;
					}
				}
			}
		}

		Data::CParams Attribute;
		Attribute.emplace_back(CStrID("Class"), std::string("Frame::CLightAttribute"));
		Attribute.emplace_back(CStrID("LightType"), LightType);
		Attribute.emplace_back(CStrID("Color"), vector4(Color.v, 3));
		Attribute.emplace_back(CStrID("Intensity"), Intensity);

		if (pLight->CastShadows.Get())
			Attribute.emplace_back(CStrID("CastShadows"), true);

		if (LightType != 0)
		{
			Attribute.emplace_back(CStrID("Range"), Range);

			if (LightType == 2)
			{
				Attribute.emplace_back(CStrID("ConeInner"), static_cast<float>(pLight->InnerAngle.Get()));
				Attribute.emplace_back(CStrID("ConeOuter"), static_cast<float>(pLight->OuterAngle.Get()));
			}
		}

		Attributes.push_back(std::move(Attribute));

		return true;
	}

	bool ExportCamera(FbxCamera* pCamera, CContext& Ctx, Data::CDataArray& Attributes)
	{
		Ctx.Log.LogDebug("Camera");

		Data::CParams Attribute;
		Attribute.emplace_back(CStrID("Class"), std::string("Frame::CCameraAttribute"));

		if (pCamera->ProjectionType.Get() == FbxCamera::eOrthogonal)
			Attribute.emplace_back(CStrID("Orthogonal"), true);

		float Width = static_cast<float>(pCamera->AspectWidth.Get());
		float Height = static_cast<float>(pCamera->AspectHeight.Get());
		switch (pCamera->AspectRatioMode.Get())
		{
			case FbxCamera::eFixedWidth:  Height *= Width; break;
			case FbxCamera::eFixedHeight: Width *= Height; break;
		}

		Attribute.emplace_back(CStrID("FOV"), static_cast<float>(pCamera->FieldOfView.Get()));
		Attribute.emplace_back(CStrID("Width"), static_cast<float>(pCamera->AspectWidth.Get()));
		Attribute.emplace_back(CStrID("Height"), static_cast<float>(pCamera->AspectHeight.Get()));
		Attribute.emplace_back(CStrID("NearPlane"), static_cast<float>(pCamera->NearPlane.Get()));
		Attribute.emplace_back(CStrID("FarPlane"), static_cast<float>(pCamera->FarPlane.Get()));

		Attributes.push_back(std::move(Attribute));

		return true;
	}

	bool ExportLODGroup(FbxLODGroup* pLODGroup, CContext& Ctx, Data::CDataArray& Attributes)
	{
		Ctx.Log.LogDebug("LOD group");

		assert(!pLODGroup->ThresholdsUsedAsPercentage.Get());
		assert(pLODGroup->WorldSpace.Get());

		const int ThresholdCount = std::min(pLODGroup->GetNumThresholds(), std::max(0, pLODGroup->GetNode()->GetChildCount() - 1));
		const bool UseMinMax = pLODGroup->MinMaxDistance.Get();

		if (!ThresholdCount && !UseMinMax)
		{
			Ctx.Log.LogWarning(std::string("LOD group ") + pLODGroup->GetName() + " has no thresholds or nodes to switch, skipped");
			return true;
		}

		std::vector<std::pair<float, std::string>> Thresholds(ThresholdCount + (UseMinMax ? 2 : 0));
		for (int i = 0; i < ThresholdCount; ++i)
		{
			FbxDistance Value;
			if (!pLODGroup->GetThreshold(i, Value))
			{
				Ctx.Log.LogWarning(std::string("LOD group ") + pLODGroup->GetName() + " has invalid threshold, skipped");
				return true;
			}

			Thresholds[i].first = Value.value();

			const FbxNode* pChild = pLODGroup->GetNode()->GetChild(i);
			Thresholds[i].second = pChild ? pChild->GetName() : std::string();
		}

		const FbxNode* pLastChild = pLODGroup->GetNode()->GetChild(pLODGroup->GetNode()->GetChildCount());
		std::string LastID = pLastChild ? pLastChild->GetName() : std::string();

		if (UseMinMax)
		{
			// .second is left empty, nothing is rendered before the MinDistance
			Thresholds[ThresholdCount].first = static_cast<float>(pLODGroup->MinDistance.Get());

			// Now the last node is rendered only before the MaxDistance
			Thresholds[ThresholdCount + 1].first = static_cast<float>(pLODGroup->MaxDistance.Get());
			Thresholds[ThresholdCount + 1].second = LastID;

			// Nothing is rendered after the MaxDistance
			LastID.clear();
		}

		std::sort(Thresholds.begin(), Thresholds.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

		Data::CParams Attribute;
		Attribute.emplace_back(CStrID("Class"), std::string("Scene::CLODGroup"));

		//!!!can't store float -> string in a Data::CParams! Use array of sections with 2 fields each?
		//!!!store the last one! //???use FLT_MAX for uniformity of the list?

		assert(false && "IMPLEMENT ME!!!");

		Attributes.push_back(std::move(Attribute));

		return true;
	}

	bool ExportAnimation(FbxAnimStack* pAnimStack, FbxScene* pScene, CContext& Ctx)
	{
		// TODO: to utility function for name validation, return bool if were changes
		std::string AnimName = pAnimStack->GetName();
		ToLower(AnimName);
		std::replace(AnimName.begin(), AnimName.end(), ' ', '_'); // Can use std::transform and the list of forbidden characters

		const auto LayerCount = pAnimStack->GetMemberCount<FbxAnimLayer>();
		if (LayerCount <= 0)
		{
			Ctx.Log.LogWarning(std::string("Animation ") + AnimName + " has no layers, skipped");
			return true;
		}
		else if (LayerCount > 1)
		{
			Ctx.Log.LogWarning(std::string("Animation ") + AnimName + " has more than one layer");
		}

		const FbxTakeInfo* pTakeInfo = pScene->GetTakeInfo(pAnimStack->GetName());
		const FbxLongLong StartFrame = pTakeInfo->mLocalTimeSpan.GetStart().GetFrameCount(FbxTime::GetGlobalTimeMode());
		const FbxLongLong EndFrame = pTakeInfo->mLocalTimeSpan.GetStop().GetFrameCount(FbxTime::GetGlobalTimeMode());

		Ctx.Log.LogDebug(std::string("Animation ") + AnimName +
			", frames " + std::to_string(StartFrame) + '-' + std::to_string(EndFrame));

		const auto pLayer = pAnimStack->GetMember<FbxAnimLayer>(0);
		if (!pLayer || !pLayer->GetMemberCount())
		{
			Ctx.Log.LogWarning(std::string("Animation ") + AnimName + " is empty, skipped");
			return true;
		}

		// TODO: can cache ACL skeletons! Build once for the whole scene, check transform animation in all stacks!

		// FIXME: ACL supports multiple root bones since 1.3.0!

		// Theoretically we can have more than one skeleton hierarchy animated with the same stack. Process them all.
		// Note that here we implicitly skip all non-transform animations, like light color animation etc.
		// TODO: support non-transform animations if necessary.
		std::vector<FbxNode*> SkeletonRoots;
		FindSkeletonRoots(pAnimStack, pScene->GetRootNode(), SkeletonRoots);
		if (SkeletonRoots.empty())
		{
			Ctx.Log.LogWarning(std::string("Animation ") + AnimName + " doesn't affect any node transformation, skipped");
			return true;
		}

		// Create rigid skeletons and associate ACL bones with FbxNode instances

		std::vector<CSkeletonACLBinding> Skeletons;
		for (FbxNode* pRoot : SkeletonRoots)
		{
			CSkeletonACLBinding Skeleton;

			std::vector<acl::RigidBone> Bones;
			BuildACLSkeleton(pRoot, Skeleton.FbxBones, Bones);

			assert(Bones.size() <= std::numeric_limits<uint16_t>().max());

			Skeleton.Skeleton = acl::make_unique<acl::RigidSkeleton>(
				Ctx.ACLAllocator, Ctx.ACLAllocator, Bones.data(), static_cast<uint16_t>(Bones.size()));

			Skeletons.push_back(std::move(Skeleton));
		}

		// Evaluate animation of all skeletons frame by frame and compress with ACL

		const auto FrameRate = static_cast<float>(FbxTime::GetFrameRate(FbxTime::GetGlobalTimeMode()));
		const auto FrameCount = static_cast<uint32_t>(EndFrame - StartFrame + 1);

		for (CSkeletonACLBinding& Skeleton : Skeletons)
		{
			// FIXME: now bone indices are per-skeleton, can't use one clip for all!
			//???must create different clips (like now) or unify bones?
			acl::String ClipName(Ctx.ACLAllocator, "Run Cycle");
			acl::AnimationClip Clip(Ctx.ACLAllocator, *Skeleton.Skeleton, FrameCount, FrameRate, ClipName);

			uint32_t SampleIndex = 0;
			FbxTime FrameTime;
			for (FbxLongLong Frame = StartFrame; Frame <= EndFrame; ++Frame, ++SampleIndex)
			{
				FrameTime.SetFrame(Frame, FbxTime::GetGlobalTimeMode());

				for (size_t BoneIdx = 0; BoneIdx < Skeleton.FbxBones.size(); ++BoneIdx)
				{
					const auto LocalTfm = Skeleton.FbxBones[BoneIdx]->EvaluateLocalTransform(FrameTime);
					const auto Scaling = LocalTfm.GetS();
					const auto Rotation = LocalTfm.GetQ();
					const auto Translation = LocalTfm.GetT();

					acl::AnimatedBone& Bone = Clip.get_animated_bone(static_cast<uint16_t>(BoneIdx));
					Bone.scale_track.set_sample(SampleIndex, { Scaling[0], Scaling[1], Scaling[2], 1.0 });
					Bone.rotation_track.set_sample(SampleIndex, { Rotation[0], Rotation[1], Rotation[2], Rotation[3] });
					Bone.translation_track.set_sample(SampleIndex, { Translation[0], Translation[1], Translation[2], 1.0 });
				}
			}

			acl::OutputStats Stats;
			acl::CompressedClip* CompressedClip = nullptr;
			acl::ErrorResult ErrorResult = acl::uniformly_sampled::compress_clip(Ctx.ACLAllocator, Clip, Ctx.ACLSettings, CompressedClip, Stats);
			if (!ErrorResult.empty())
			{
				Ctx.Log.LogWarning(std::string("ACL failed to compress animation ") + AnimName + " for one of skeletons");
				continue;
			}

			Ctx.Log.LogDebug(std::string("ACL compressed animation ") + AnimName + " to " + std::to_string(CompressedClip->get_size()) + " bytes");

			{
				const auto DestPath = Ctx.AnimPath / (AnimName + ".anm");

				fs::create_directories(Ctx.AnimPath);

				//???save bone mapping? name to index, like in skins. ACL doesn't know how to associate bones with tracks by name.

				std::ofstream File(DestPath, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
				if (!File)
				{
					Ctx.Log.LogError("Error opening an output file " + DestPath.string());
					return false;
				}

				File.write(reinterpret_cast<const char*>(CompressedClip), CompressedClip->get_size());
			}
		}

		return true;
	}

	// FbxAnimUtilities::IsChannelAnimated is broken completely and FbxAnimUtilities::IsAnimated is an overshot.
	// Also this one allows us to check per clip (animation stack).
	static bool IsPropertyAnimated(FbxAnimStack* pAnimStack, FbxProperty& Property)
	{
		const auto LayerCount = pAnimStack->GetMemberCount<FbxAnimLayer>();
		for (int i = 0; i < LayerCount; ++i)
		{
			auto pLayer = pAnimStack->GetMember<FbxAnimLayer>(i);
			if (Property.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_X) ||
				Property.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Y) ||
				Property.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Z))
			{
				return true;
			}
		}
		return false;
	}

	void FindSkeletonRoots(FbxAnimStack* pAnimStack, FbxNode* pNode, std::vector<FbxNode*>& SkeletonRoots)
	{
		if (IsPropertyAnimated(pAnimStack, pNode->LclScaling) ||
			IsPropertyAnimated(pAnimStack, pNode->LclRotation) ||
			IsPropertyAnimated(pAnimStack, pNode->LclTranslation))
		{
			SkeletonRoots.push_back(pNode);
			return;
		}

		for (int i = 0; i < pNode->GetChildCount(); ++i)
			FindSkeletonRoots(pAnimStack, pNode->GetChild(i), SkeletonRoots);
	}

	void BuildACLSkeleton(FbxNode* pNode, std::vector<FbxNode*>& FbxBones, std::vector<acl::RigidBone>& Bones)
	{
		acl::RigidBone Bone;
		if (FbxBones.empty())
		{
			Bone.parent_index = acl::k_invalid_bone_index;
		}
		else
		{
			auto It = std::find(FbxBones.crbegin(), FbxBones.crend(), pNode->GetParent());
			assert(It != FbxBones.crend());
			Bone.parent_index = static_cast<uint16_t>(std::distance(FbxBones.cbegin(), It.base()) - 1);
		}

		// TODO: metric from per-bone AABBs?
		Bone.vertex_distance = 3.f;

		FbxBones.push_back(pNode);
		Bones.push_back(std::move(Bone));

		for (int i = 0; i < pNode->GetChildCount(); ++i)
		{
			// Traverse only through bones
			for (int i = 0; i < pNode->GetNodeAttributeCount(); ++i)
			{
				if (pNode->GetNodeAttributeByIndex(i)->GetAttributeType() == FbxNodeAttribute::eSkeleton)
				{
					BuildACLSkeleton(pNode->GetChild(i), FbxBones, Bones);
					break;
				}
			}
		}
	}
};

int main(int argc, const char** argv)
{
	CFBXTool Tool("cf-fbx", "FBX to DeusExMachina resource converter", { 1, 0, 0 });
	return Tool.Execute(argc, argv);
}

// =====================================================
// There are different ways to get node animation data

// With limits, pivots etc:
//pNode->EvaluateLocalTransform(FrameTime);

// With limits, but no pivots etc:
//pNode->EvaluateLocalRotation(FrameTime);

// Property only:
//pNode->LclTranslation.EvaluateValue(FrameTime);

// Property per component per animation layer (sample curve directly):
//if (const auto pAnimCurve = pNode->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_X))
//	pAnimCurve->Evaluate(FrameTime);
// =====================================================

/*

//-----------------------------------------------------------------------

DecompressionContext<DefaultDecompressionSettings> context;
//DecompressionSettings::disable_fp_exceptions()

context.initialize(*compressed_clip);
context.seek(sample_time, SampleRoundingPolicy::None);

context.decompress_bone(bone_index, &rotation, &translation, &scale);

//is_dirty(const CompressedClip& clip)
//make_decompression_context(...)

//Every decompression function supported by the context is prefixed with decompress_.
//Uniform sampling supports decompressing a whole pose with a custom OutputWriter for
//optimized pose writing. You can implement your own and coerce to your own math types.
//The type is templated on the decompress_pose function in order to be easily inlined.
Transform_32* transforms = new Transform_32[num_bones];
DefaultOutputWriter pose_writer(transforms, num_bones);
context.decompress_pose(pose_writer);


*/
