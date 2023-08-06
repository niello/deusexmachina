#include <ContentForgeTool.h>
#include <SceneTools.h>
#include <Render/ShaderMetaCommon.h>
#include <Utils.h>
#include <ParamsUtils.h>
#include <CLI11.hpp>
#include <fbxsdk.h>
#include <acl/core/ansi_allocator.h>
#include <acl/compression/animation_clip.h>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/scenes --path Data ../../../content

// NB: FBX SDK supports fbx, dxf, dae, obj and 3ds, so cf-fbx must be able to read all of them too.

// Blender v2.90 export settings:
// - Path Mode: Copy, Embed Textures (recommended for material export, but you can try separated texture files too)
// - Custom Properties: on
// - Apply Scalings: FBX Units Scale / FBX All
// - Forward: -Z
// - Up: Y
// - Apply Unit: off
// - Apply Transform: off (if on, node transforms are closer to original, but somethings else seems to be baked into geometry)

template<class TDEM, class TFBX>
static TDEM FbxToDEM(const TFBX& Value)
{
	static_assert(false, "FbxToDEM not implemented for these types!");
}
//---------------------------------------------------------------------

template<>
static inline float2 FbxToDEM(const FbxVector2& Value)
{
	return float2{ static_cast<float>(Value[0]), static_cast<float>(Value[1]) };
}
//---------------------------------------------------------------------

template<>
static inline float3 FbxToDEM(const FbxDouble3& Value)
{
	return float3{ static_cast<float>(Value[0]), static_cast<float>(Value[1]), static_cast<float>(Value[2]) };
}
//---------------------------------------------------------------------

template<>
static inline float3 FbxToDEM(const FbxVector4& Value)
{
	return float3{ static_cast<float>(Value[0]), static_cast<float>(Value[1]), static_cast<float>(Value[2]) };
}
//---------------------------------------------------------------------

template<>
static inline float4 FbxToDEM(const FbxVector4& Value)
{
	return float4{ static_cast<float>(Value[0]), static_cast<float>(Value[1]), static_cast<float>(Value[2]), static_cast<float>(Value[3]) };
}
//---------------------------------------------------------------------

template<>
static inline float4 FbxToDEM(const FbxColor& Value)
{
	return float4{ static_cast<float>(Value[0]), static_cast<float>(Value[1]), static_cast<float>(Value[2]), static_cast<float>(Value[3]) };
}
//---------------------------------------------------------------------

static inline float3 FbxToDEMVec3(const FbxDouble3& Value)
{
	return float3{ static_cast<float>(Value[0]), static_cast<float>(Value[1]), static_cast<float>(Value[2]) };
}
//---------------------------------------------------------------------

static inline float4 FbxToDEMVec4(const FbxDouble4& Value)
{
	return float4{ static_cast<float>(Value[0]), static_cast<float>(Value[1]), static_cast<float>(Value[2]), static_cast<float>(Value[3]) };
}
//---------------------------------------------------------------------

static inline FbxAMatrix GetFbxNodeGeometricTransform(FbxNode* pNode)
{
	return FbxAMatrix(pNode->GetGeometricTranslation(FbxNode::eSourcePivot),
		pNode->GetGeometricRotation(FbxNode::eSourcePivot),
		pNode->GetGeometricScaling(FbxNode::eSourcePivot));
}
//---------------------------------------------------------------------

static FbxAMatrix GetFbxNodeGlobalPosition(FbxNode* pNode, FbxPose* pPose)
{
	if (pPose)
	{
		const int NodeIndex = pPose->Find(pNode);

		if (NodeIndex > -1)
		{
			const FbxMatrix& PoseMatrix = pPose->GetMatrix(NodeIndex);
			FbxAMatrix Result;
			memcpy((double*)Result.mData, (const double*)PoseMatrix.mData, 16 * sizeof(double));

			// The bind pose is always a global matrix, but the rest pose may not be
			if (!pPose->IsBindPose() && pPose->IsLocalMatrix(NodeIndex) && pNode->GetParent())
				Result = GetFbxNodeGlobalPosition(pNode->GetParent(), pPose) * Result;

			return Result;
		}
	}

	return pNode->EvaluateGlobalTransform();
}
//---------------------------------------------------------------------

static void GetAbsoluteFbxNodePath(const FbxNode* pNode, std::vector<std::string>& OutPath)
{
	const auto* pCurrNode = pNode;
	while (pCurrNode)
	{
		OutPath.push_back(pCurrNode->GetName());
		pCurrNode = pCurrNode->GetParent();
	}
}
//---------------------------------------------------------------------

static size_t GetFbxNodeDepth(const FbxNode* pNode)
{
	size_t Depth = 0;
	const auto* pCurrNode = pNode;
	while (pCurrNode)
	{
		++Depth;
		pCurrNode = pCurrNode->GetParent();
	}
	return Depth;
}
//---------------------------------------------------------------------

static const FbxNode* FindLastCommonFbxNode(const FbxNode* pNodeA, const FbxNode* pNodeB)
{
	std::vector<const FbxNode*> ChainA;
	const FbxNode* pCurrNode = pNodeA;
	while (pCurrNode)
	{
		if (pCurrNode == pNodeB) return pNodeB;
		ChainA.push_back(pCurrNode);
		pCurrNode = pCurrNode->GetParent();
	}
	ChainA.push_back(nullptr);
	std::reverse(ChainA.begin(), ChainA.end());

	std::vector<const FbxNode*> ChainB;
	pCurrNode = pNodeB;
	while (pCurrNode)
	{
		if (pCurrNode == pNodeA) return pNodeA;
		ChainB.push_back(pCurrNode);
		pCurrNode = pCurrNode->GetParent();
	}
	ChainB.push_back(nullptr);
	std::reverse(ChainB.begin(), ChainB.end());

	// Start from 
	const size_t Depth = std::min(ChainA.size(), ChainB.size());
	for (size_t i = 1; i < Depth; ++i)
		if (ChainA[i] != ChainB[i]) return ChainA[i - 1];
	return ChainA[Depth - 1];
}
//---------------------------------------------------------------------

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

	OutValue = FbxToDEM<TOut>(pElement->GetDirectArray().GetAt(ID));
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

static acl::Transform_32 GetNodeTransform(FbxNode* pNode)
{
	// Raw properties are left for testing purposes
	constexpr bool UseRawProperties = false;
	if (UseRawProperties)
	{
		const auto T = pNode->LclTranslation.Get();
		const auto S = pNode->LclScaling.Get();

		FbxQuaternion R;
		R.ComposeSphericalXYZ(pNode->LclRotation.Get());

		return
		{
			{ static_cast<float>(R[0]), static_cast<float>(R[1]), static_cast<float>(R[2]), static_cast<float>(R[3]) },
			{ static_cast<float>(T[0]), static_cast<float>(T[1]), static_cast<float>(T[2]), 1.f },
			{ static_cast<float>(S[0]), static_cast<float>(S[1]), static_cast<float>(S[2]), 0.f }
		};
	}
	else
	{
		const auto LocalTfm = pNode->EvaluateLocalTransform();
		const auto R = LocalTfm.GetQ();
		const auto T = LocalTfm.GetT();
		const auto S = LocalTfm.GetS();

		return
		{
			{ static_cast<float>(R[0]), static_cast<float>(R[1]), static_cast<float>(R[2]), static_cast<float>(R[3]) },
			{ static_cast<float>(T[0]), static_cast<float>(T[1]), static_cast<float>(T[2]), 1.f },
			{ static_cast<float>(S[0]), static_cast<float>(S[1]), static_cast<float>(S[2]), 0.f }
		};
	}
}
//---------------------------------------------------------------------

class CFBXTool : public CContentForgeTool
{
protected:

	struct CContext
	{
		CThreadSafeLog&     Log;
		Data::CParams&      TaskParams;

		acl::ANSIAllocator  ACLAllocator;

		fs::path            ScenePath;
		fs::path            MeshPath;
		fs::path            MaterialPath;
		fs::path            TexturePath;
		fs::path            SkinPath;
		fs::path            AnimPath;
		fs::path            CollisionPath;
		std::string         TaskName;
		Data::CParamsSorted MaterialMap;

		std::string         LeftFootBoneName;
		std::string         RightFootBoneName;

		std::unordered_map<const FbxMesh*, CMeshAttrInfo> ProcessedMeshes;
		std::unordered_map<const FbxMesh*, CSkinAttrInfo> ProcessedSkins;
		std::unordered_map<const FbxTexture*, std::string> ProcessedTextures;
	};

	struct CSkeletonACLBinding
	{
		acl::RigidSkeletonPtr Skeleton;
		std::vector<FbxNode*> Nodes; // Indices in this array are used as bone indices in ACL
	};

	FbxManager*                pFBXManager = nullptr;
	Data::CSchemeSet          _SceneSchemes;
	CSceneSettings            _Settings;

	std::string               _ResourceRoot;
	std::string               _SchemeFile;
	std::string               _SettingsFile;
	double                    _AnimSamplingRate = 30.0;
	bool                      _OutputBin = false;
	bool                      _OutputHRD = false; // For debug purposes, saves scene hierarchies in a human-readable format

public:

	CFBXTool(const std::string& Name, const std::string& Desc, CVersion Version)
		: CContentForgeTool(Name, Desc, Version)
		, _ResourceRoot("Data:")
	{
		// Set default before parsing command line
		_SchemeFile = "../schemes/scene.dss";
		_SettingsFile = "../schemes/settings.hrd";

		InitImageProcessing();
	}

	~CFBXTool()
	{
		TermImageProcessing();
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
			if (!ParamsUtils::LoadSchemes(_SchemeFile.c_str(), _SceneSchemes))
			{
				std::cout << "Couldn't load scene binary serialization scheme from " << _SchemeFile;
				return 2;
			}
		}

		if (!LoadSceneSettings(_SettingsFile, _Settings)) return 3;

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
		CLIApp.add_option("--settings", _SettingsFile, "Settings file path");
		CLIApp.add_option("--fps", _AnimSamplingRate, "Animation sampling rate in frames per second, default is 30");
		CLIApp.add_flag("-t,--txt", _OutputHRD, "Output scenes in a human-readable format, suitable for debugging only");
		CLIApp.add_flag("-b,--bin", _OutputBin, "Output scenes in a binary format, suitable for loading into the engine");
	}

	virtual ETaskResult ProcessTask(CContentForgeTask& Task) override
	{
		const auto Extension = Task.SrcFilePath.extension();
		if (Extension != ".fbx")
		{
			Task.Log.LogWarning("Filename extension must be .fbx");
			return ETaskResult::NotStarted;
		}

		// Import FBX scene from the source file

		auto FBMPath = Task.SrcFilePath;
		FBMPath.replace_extension("fbm");
		const bool HasFBM = fs::exists(FBMPath);

		FbxImporter* pImporter = FbxImporter::Create(pFBXManager, "");

		const auto SrcPath = Task.SrcFilePath.string();

		if (!pImporter->Initialize(SrcPath.c_str(), -1, pFBXManager->GetIOSettings()))
		{
			Task.Log.LogError("Failed to create FbxImporter for " + SrcPath + ": " + pImporter->GetStatus().GetErrorString());
			pImporter->Destroy();
			return ETaskResult::Failure;
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
			return ETaskResult::Failure;
		}

		pImporter->Destroy();

		// Convert scene transforms and geometry to a more suitable format

		//auto AxisSystem = pScene->GetGlobalSettings().GetAxisSystem();
		//auto OriginalUpAxis = pScene->GetGlobalSettings().GetOriginalUpAxis();

		// Setup scene unit, DEM uses meters, but FBX default is centimeters
		if (pScene->GetGlobalSettings().GetSystemUnit() != FbxSystemUnit::m)
		{
			Task.Log.LogInfo("Scene unit converted from " + std::string(pScene->GetGlobalSettings().GetSystemUnit().GetScaleFactorAsString()) + " to meters");

			FbxSystemUnit::ConversionOptions Options;
			Options.mConvertRrsNodes = false;
			Options.mConvertLimits = true;
			Options.mConvertClusters = true;
			Options.mConvertLightIntensity = false; // TODO: check if 'true' needed
			Options.mConvertPhotometricLProperties = true;
			Options.mConvertCameraClipPlanes = true;

			FbxSystemUnit::m.ConvertScene(pScene, Options);
		}

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
		CContext Ctx{ Task.Log, Task.Params };

		Ctx.MaterialMap = ParamsUtils::OrderedParamsToSorted(ParamsUtils::GetParam<Data::CParams>(Task.Params, "Materials", {}));

		Ctx.TaskName = Task.TaskID.CStr();

		Ctx.ScenePath = GetPath(Task.Params, "Output");
		Ctx.MeshPath = GetPath(Task.Params, "MeshOutput");
		Ctx.MaterialPath = GetPath(Task.Params, "MaterialOutput");
		Ctx.TexturePath = GetPath(Task.Params, "TextureOutput");
		Ctx.SkinPath = GetPath(Task.Params, "SkinOutput");
		Ctx.AnimPath = GetPath(Task.Params, "AnimOutput");
		Ctx.CollisionPath = GetPath(Task.Params, "CollisionOutput");

		// Export node hierarchy to DEM format, omit FBX root node
		
		Data::CParams Nodes;

		// Must be identity, but let's ensure all is correct
		const auto RootTfm = GetNodeTransform(pScene->GetRootNode());

		const Data::CDataArray* pNodes = nullptr;
		if (ParamsUtils::TryGetParam(pNodes, Ctx.TaskParams, "Nodes"))
		{
			assert(false && "NOT IMPLEMENTED!!!");
			//// Export only selected nodes
			//for (const auto& NodePathData : *pNodes)
			//	if (auto pNode = FindNodeByPath(Ctx.Doc, Scene, NodePathData.GetValue<std::string>()))
			//		if (!ExportNode(pNode->id, Ctx, Nodes, acl::transform_identity_32()))
			//			return ETaskResult::Failure;
		}
		else
		{
			// Export all nodes
			for (int i = 0; i < pScene->GetRootNode()->GetChildCount(); ++i)
				if (!ExportNode(pScene->GetRootNode()->GetChild(i), Ctx, Nodes, RootTfm))
				{
					return ETaskResult::Failure;
				}
		}

		// Export animations

		// Each stack is a clip. Only one layer (or combined result of all stack layers) is considered, no
		// blending info saved. Nodes are processed from the root recursively and each can have up to 3 tracks.
		if (!Ctx.AnimPath.empty())
		{
			Ctx.LeftFootBoneName = ParamsUtils::GetParam(Task.Params, "LeftFootBoneName", std::string{});
			Ctx.RightFootBoneName = ParamsUtils::GetParam(Task.Params, "RightFootBoneName", std::string{});

			const int AnimationCount = pScene->GetSrcObjectCount<FbxAnimStack>();
			for (int i = 0; i < AnimationCount; ++i)
			{
				const auto pAnimStack = static_cast<FbxAnimStack*>(pScene->GetSrcObject<FbxAnimStack>(i));
				if (!ExportAnimation(pAnimStack, pScene, Ctx))
				{
					return ETaskResult::Failure;
				}
			}
		}

		// Erase .fbm folder extracted by SDK because it doesn't cleanup after itself as of 2020.0
		//!!!TODO: also erase when fail during the task!
		if (!HasFBM) fs::remove_all(FBMPath);

		// Export additional info

		// pScene->GetGlobalSettings().GetAmbientColor();
		// Scene->GetPoseCount();

		// Finalize and save the scene

		const bool CreateRoot = ParamsUtils::GetParam(Ctx.TaskParams, "CreateRoot", true);

		bool Result = true;
		if (!Ctx.ScenePath.empty())
			Result = WriteDEMScene(Ctx.ScenePath, Task.TaskID.ToString(), std::move(Nodes), _SceneSchemes, Task.Params,
				_OutputHRD, _OutputBin, CreateRoot, Task.Log);
		return Result ? ETaskResult::Success : ETaskResult::Failure;
	}

	bool ExportNode(FbxNode* pNode, CContext& Ctx, Data::CParams& Nodes, const acl::Transform_32& ParentGlobalTfm)
	{
		if (!pNode)
		{
			Ctx.Log.LogWarning("Nullptr FBX node encountered");
			return true;
		}

		Ctx.Log.LogDebug(std::string("Node ") + pNode->GetName());

		static const CStrID sidAttrs("Attrs");
		static const CStrID sidChildren("Children");

		Data::CParams NodeSection;

		// Process transform

		const auto Tfm = GetNodeTransform(pNode);
		const auto GlobalTfm = acl::transform_mul(Tfm, ParentGlobalTfm);

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
					if (!ExportModel(static_cast<FbxMesh*>(pAttribute), Ctx, Attributes, GlobalTfm)) return false;
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

		// Process transform

		// Bone transformation is determined by the bind pose and animations
		// TODO: use AutocreateBones when skeleton has no attachments?
		if (!IsBone)
			FillNodeTransform(Tfm, NodeSection);

		// Write attributes after SRT for better readability

		if (!Attributes.empty())
			NodeSection.emplace_back(sidAttrs, std::move(Attributes));

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

			if (!ExportNode(pNode->GetChild(i), Ctx, Children, GlobalTfm)) return false;
		}

		if (!Children.empty())
			NodeSection.emplace_back(sidChildren, std::move(Children));

		auto NodeName = GetValidNodeName(pNode->GetName());
		CStrID NodeID = CStrID(NodeName);
		if (ParamsUtils::HasParam(Nodes, NodeID))
			Ctx.Log.LogWarning("Duplicated node overwritten with name " + NodeName);

		Nodes.emplace_back(NodeID, std::move(NodeSection));

		return true;
	}

	bool ExportModel(const FbxMesh* pMesh, CContext& Ctx, Data::CDataArray& Attributes, const acl::Transform_32& GlobalTfm)
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
		for (const std::string& MaterialID : MeshInfo.MaterialIDs)
		{
			// Assemble a model attribute per submesh

			Data::CParams ModelAttribute;
			ModelAttribute.emplace_back(CStrID("Class"), 'MDLA'); //std::string("Frame::CModelAttribute"));
			ModelAttribute.emplace_back(CStrID("Mesh"), MeshInfo.MeshID);
			if (GroupIndex > 0)
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
		if (SkinIt != Ctx.ProcessedSkins.cend())
		{
			if (!SkinIt->second.SkinID.empty())
			{
				Data::CParams SkinAttribute;
				SkinAttribute.emplace_back(CStrID("Class"), 'SKIN'); // Frame::CSkinAttribute
				SkinAttribute.emplace_back(CStrID("SkinInfo"), SkinIt->second.SkinID);
				if (!SkinIt->second.RootSearchPath.empty())
					SkinAttribute.emplace_back(CStrID("RootSearchPath"), SkinIt->second.RootSearchPath);
				//SkinAttribute.emplace(CStrID("AutocreateBones"), true);
				Attributes.push_back(std::move(SkinAttribute));
			}
		}

		// Create collision shape if necessary

		auto DEM_collision = pMesh->FindProperty("DEM_collision");
		if (DEM_collision.IsValid())
		{
			std::string ShapeType = DEM_collision.Get<FbxString>();
			const std::string MeshName = pMesh->GetName();
			const auto MeshRsrcName = GetValidResourceName(MeshName.empty() ? Ctx.TaskName + '_' + MeshName : MeshName);

			const auto ShapePath = GenerateCollisionShape(ShapeType, Ctx.CollisionPath, MeshRsrcName, MeshInfo, GlobalTfm, _PathAliases, Ctx.Log);
			if (!ShapePath.has_value()) return false; //???warn instead of failing and proceed without a shape?

			// Don't create a collision shape for rigid bodies
			const bool IsRigidBody = ParamsUtils::GetParam(Ctx.TaskParams, "RigidBody", false);
			if (!ShapePath.value().empty() && !IsRigidBody)
			{
				const auto ShapeID = _ResourceRoot + fs::relative(ShapePath.value(), _RootDir).generic_string();

				Data::CParams CollisionAttribute;
				CollisionAttribute.emplace_back(CStrID("Class"), 'COLA'); // Physics::CCollisionAttribute
				CollisionAttribute.emplace_back(CStrID("Shape"), ShapeID);
				//CollisionAttribute.emplace_back(CStrID("Static"), true);
				//CollisionAttribute.emplace_back(CStrID("CollisionGroup"), CStrID("..."));
				//CollisionAttribute.emplace_back(CStrID("CollisionMask"), CStrID("..."));
				Attributes.push_back(std::move(CollisionAttribute));
			}
		}

		return true;
	}

	// Export mesh and optional skin to DEM-native format
	bool ExportMesh(const FbxMesh* pMesh, CContext& Ctx)
	{
		// Determine vertex format

		CVertexFormat VertexFormat;
		VertexFormat.BlendWeightSize = 32;

		VertexFormat.NormalCount = std::min(1, pMesh->GetElementNormalCount());
		if (pMesh->GetElementNormalCount() > static_cast<int>(VertexFormat.NormalCount))
			Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " will use only " + std::to_string(VertexFormat.NormalCount) + '/' + std::to_string(pMesh->GetElementNormalCount()) + " normals");

		VertexFormat.TangentCount = std::min(1, pMesh->GetElementTangentCount());
		if (pMesh->GetElementTangentCount() > static_cast<int>(VertexFormat.TangentCount))
			Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " will use only " + std::to_string(VertexFormat.TangentCount) + '/' + std::to_string(pMesh->GetElementTangentCount()) + " tangents");

		VertexFormat.BitangentCount = std::min(1, pMesh->GetElementBinormalCount());
		if (pMesh->GetElementBinormalCount() > static_cast<int>(VertexFormat.BitangentCount))
			Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " will use only " + std::to_string(VertexFormat.BitangentCount) + '/' + std::to_string(pMesh->GetElementBinormalCount()) + " bitangents");

		VertexFormat.ColorCount = std::min(1, pMesh->GetElementVertexColorCount());
		if (pMesh->GetElementVertexColorCount() > static_cast<int>(VertexFormat.ColorCount))
			Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " will use only " + std::to_string(VertexFormat.ColorCount) + '/' + std::to_string(pMesh->GetElementVertexColorCount()) + " colors");

		VertexFormat.UVCount = std::min(static_cast<int>(MaxUV), pMesh->GetElementUVCount());
		if (pMesh->GetElementUVCount() > static_cast<int>(VertexFormat.UVCount))
			Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " will use only " + std::to_string(VertexFormat.UVCount) + '/' + std::to_string(pMesh->GetElementUVCount()) + " UVs");

		const auto MaterialLayerCount = std::min(1, pMesh->GetElementMaterialCount());
		if (pMesh->GetElementMaterialCount() > MaterialLayerCount)
			Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " will use only " + std::to_string(MaterialLayerCount) + '/' + std::to_string(pMesh->GetElementMaterialCount()) + " material layers");

		// TODO: process blend shapes!

		// Collect mesh skin data, one skin for all submeshes

		struct CFbxBoneInfo
		{
			const FbxCluster*                  pCluster = nullptr;
			size_t                             Depth = 0;
			size_t                             AffectedVertexCount = 0;
			std::vector<std::pair<int, float>> WeightByControlPoint; // Sorted by index for fast search
			uint16_t                           ParentIndex = NoParentBone;
			bool                               HasInfluence = false;
		};

		// TODO: merge skins with the same root? many meshes can share one skin!
		std::vector<CFbxBoneInfo> FbxBones;
		const FbxNode* pSkinRoot = nullptr;
		bool HasSkinInfluence = false;

		const int SkinCount = pMesh->GetDeformerCount(FbxDeformer::eSkin);
		assert(SkinCount < 2); // TODO: check how it will work with multiple skins
		for (int s = 0; s < SkinCount; ++s)
		{
			const FbxSkin* pSkin = static_cast<FbxSkin*>(pMesh->GetDeformer(s, FbxDeformer::eSkin));
			const auto ClusterCount = pSkin->GetClusterCount();

			FbxBones.reserve(FbxBones.size() + ClusterCount);

			for (int c = 0; c < ClusterCount; ++c)
			{
				const FbxCluster* pCluster = pSkin->GetCluster(c);
				const FbxNode* pBone = pCluster->GetLink();
				if (!pBone)
				{
					Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " has null bone");
					continue;
				}

				pSkinRoot = pSkinRoot ? FindLastCommonFbxNode(pSkinRoot, pBone) : pBone;

				FbxBones.push_back({});
				FbxBones.back().pCluster = pCluster;
				FbxBones.back().Depth = GetFbxNodeDepth(pBone);

				// We must find the last common node for all influencing bones to build valid skin from there
				if (pCluster->GetControlPointIndices() &&
					pCluster->GetControlPointWeights() &&
					pCluster->GetControlPointIndicesCount())
				{
					FbxBones.back().HasInfluence = true;
					HasSkinInfluence = true;
				}
			}
		}

		// Process and optimize collected skin data

		std::vector<CBone> Bones;
		if (HasSkinInfluence)
		{
			// Order active bones by increasing depth
			std::stable_sort(FbxBones.begin(), FbxBones.end(), [](const auto& a, const auto& b) { return a.Depth < b.Depth; });

			// Establish parent-child links
			auto ItParentsStart = FbxBones.begin();
			auto ItParentsEnd = FbxBones.begin() + 1;
			for (auto It = FbxBones.begin() + 1; It != FbxBones.end(); ++It)
			{
				// Find range of our possible parents by depth value
				const auto ParentDepth = It->Depth - 1;
				if (ItParentsStart->Depth < ParentDepth)
				{
					ItParentsStart = std::lower_bound(ItParentsEnd, It, ParentDepth,
						[](const auto& BoneInfo, size_t Value) { return BoneInfo.Depth < Value; });
					ItParentsEnd = It;
				}

				// Try to find a parent in range
				const FbxNode* pParentNode = It->pCluster->GetLink()->GetParent();
				const auto ItParent = std::find_if(ItParentsStart, ItParentsEnd, [pParentNode](const auto& Pair)
				{
					return Pair.pCluster->GetLink() == pParentNode;
				});

				if (ItParent == ItParentsEnd)
					Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " bone " + It->pCluster->GetLink()->GetName() + " parent not found, skin may be broken");
				else
					It->ParentIndex = static_cast<uint16_t>(std::distance(FbxBones.begin(), ItParent));
			}

			// Mark branches with influencing bones
			for (auto It = FbxBones.rbegin(); It != FbxBones.rend(); ++It)
				if (It->HasInfluence && It->ParentIndex != NoParentBone)
					FbxBones[It->ParentIndex].HasInfluence = true;

			// Filter out bones without influence, shift parent indices
			const size_t PrevSize = FbxBones.size();
			for (auto It = FbxBones.begin(); It != FbxBones.end(); /**/)
			{
				if (It->HasInfluence)
				{
					++It;
				}
				else
				{
					const uint16_t DeletedIdx = static_cast<uint16_t>(std::distance(FbxBones.begin(), It));
					It = FbxBones.erase(It);
					for (auto ItFix = It; ItFix != FbxBones.end(); ++ItFix)
						if (ItFix->ParentIndex > DeletedIdx)
							--ItFix->ParentIndex;
				}
			}
			if (FbxBones.size() < PrevSize)
				Ctx.Log.LogInfo(std::string("Mesh ") + pMesh->GetName() + " - " + std::to_string(PrevSize - FbxBones.size()) + " non-influencing leaf bones are discarded");

			// Transfer parent indices into CBone array, calculate bind pose matrices and check bone names
			Bones.reserve(FbxBones.size());
			std::set<std::string> BoneNames;
			const FbxAMatrix InvMeshWorldMatrix = pMesh->GetNode()->EvaluateGlobalTransform().Inverse();
			for (auto& BoneInfo : FbxBones)
			{
				Bones.push_back({});
				auto& Bone = Bones.back();
				Bone.ParentBoneIndex = BoneInfo.ParentIndex;

				// Generate valid name and verify its uniqueness
				//???separately check if the same FbxNode* occurs multiple times? Note that
				//GetValidNodeName may return the same result for different nodes! 
				Bone.ID = GetValidNodeName(BoneInfo.pCluster->GetLink()->GetName());
				if (BoneNames.find(Bone.ID) != BoneNames.cend())
					Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " bone " + Bone.ID + " occurs more than once, skin may be broken");
				else
					BoneNames.insert(Bone.ID);

				// TODO: pCluster->GetLinkMode() == FbxCluster::eAdditive, see ViewScene sample in a FBX SDK

				// Calculate bind pose matrix (both DEM and FBX SDK use column-major)
				FbxAMatrix BoneWorldMatrix;
				BoneInfo.pCluster->GetTransformLinkMatrix(BoneWorldMatrix);
				FbxAMatrix InvLocalBindPose = (InvMeshWorldMatrix * BoneWorldMatrix).Inverse();
				for (int Col = 0; Col < 4; ++Col)
					for (int Row = 0; Row < 4; ++Row)
						Bone.InvLocalBindPose[Col * 4 + Row] = static_cast<float>(InvLocalBindPose[Col][Row]);

				// Gather bone weights per vertex (control point)
				const int IndexCount = BoneInfo.pCluster->GetControlPointIndicesCount();
				if (BoneInfo.HasInfluence && IndexCount)
				{
					const int* pIndices = BoneInfo.pCluster->GetControlPointIndices();
					const double* pWeights = BoneInfo.pCluster->GetControlPointWeights();
					auto& Weights = BoneInfo.WeightByControlPoint;
					Weights.reserve(IndexCount);
					for (int ControlPtIdx = 0; ControlPtIdx < IndexCount; ++ControlPtIdx)
					{
						const float Weight = static_cast<float>(pWeights[ControlPtIdx]);
						if (!CompareFloat(Weight, 0.f)) Weights.emplace_back(pIndices[ControlPtIdx], Weight);
					}
					assert(!Weights.empty());
					std::sort(Weights.begin(), Weights.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

					// TODO: could optionally calculate per-bone AABB (for ragdoll etc). May be also useful for ACL.
				}
			}
		}
		else if (SkinCount)
		{
			Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " bones have no common root node, skin discarded as it may be broken");
		}

		// Collect vertices for each material separately (split mesh to groups)

		const int PolyCount = pMesh->GetPolygonCount();
		const auto pMaterialElement = pMesh->GetElementMaterial();

		std::map<std::string, CMeshGroup> SubMeshes;

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

			// Get mesh group for the current polygon material

			std::string MaterialName;
			if (pMaterialElement)
			{
				int MaterialIndex = -1;
				switch (pMaterialElement->GetMappingMode())
				{
					case FbxLayerElement::eByPolygon: MaterialIndex = pMaterialElement->GetIndexArray().GetAt(p); break;
					case FbxLayerElement::eAllSame: MaterialIndex = pMaterialElement->GetIndexArray().GetAt(0); break;
					default: assert(false && "Unsupported material mapping mode");
				}

				auto pMaterial = pMesh->GetNode()->GetMaterial(MaterialIndex);
				MaterialName = pMaterial ? pMaterial->GetName() : "";
			}

			auto GroupIt = SubMeshes.find(MaterialName);
			if (GroupIt == SubMeshes.cend())
			{
				GroupIt = SubMeshes.emplace(MaterialName, CMeshGroup{}).first;
				GroupIt->second.Vertices.reserve(static_cast<size_t>((PolyCount - p) * 3));
			}

			// Process polygon vertices
			// TODO: process each vertex once and reuse by indexing preprocessed vector?

			auto& Vertices = GroupIt->second.Vertices;
			const FbxVector4* pControlPoints = pMesh->GetControlPoints();

			for (int v = 0; v < PolySize; ++v)
			{
				const auto VertexIndex = static_cast<unsigned int>(Vertices.size());
				const auto ControlPointIndex = pMesh->GetPolygonVertex(p, v);

				CVertex Vertex;

				// NB: we need float3 positions for meshopt_optimizeOverdraw
				Vertex.Position = FbxToDEMVec3(pControlPoints[ControlPointIndex]);

				if (VertexFormat.NormalCount)
					GetVertexElement(Vertex.Normal, pMesh->GetElementNormal(), ControlPointIndex, VertexIndex);

				if (VertexFormat.TangentCount)
					GetVertexElement(Vertex.Tangent, pMesh->GetElementTangent(), ControlPointIndex, VertexIndex);

				if (VertexFormat.BitangentCount)
					GetVertexElement(Vertex.Bitangent, pMesh->GetElementBinormal(), ControlPointIndex, VertexIndex);

				if (VertexFormat.ColorCount)
				{
					float4 Color;
					GetVertexElement(Color, pMesh->GetElementVertexColor(), ControlPointIndex, VertexIndex);
					Vertex.Color = ColorRGBANorm(Color.x, Color.y, Color.z, Color.w);
				}

				for (size_t e = 0; e < VertexFormat.UVCount; ++e)
					GetVertexElement(Vertex.UV[e], pMesh->GetElementUV(e), ControlPointIndex, VertexIndex);

				// Fill skin data from influencing bones
				
				if (VertexFormat.BlendWeightSize != 8 && VertexFormat.BlendWeightSize != 32)
					Ctx.Log.LogWarning("Unsupported blend weight size, defaulting to full-precision floats (32). Supported values are 8/32.");

				for (size_t BoneIndex = 0; BoneIndex < FbxBones.size(); ++BoneIndex)
				{
					// Check if this bone influences the current vertex (control point)
					const auto& Weights = FbxBones[BoneIndex].WeightByControlPoint;
					auto WeightIt = std::lower_bound(Weights.cbegin(), Weights.cend(), ControlPointIndex,
						[](const auto& Elm, int Value) { return Elm.first < Value; });
					if (WeightIt == Weights.cend() || WeightIt->first != ControlPointIndex) continue;

					const float Weight = WeightIt->second;

					size_t BoneOrderNumber;
					if (Vertex.BonesUsed >= MaxBonesPerVertex)
					{
						Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " control point " + std::to_string(ControlPointIndex) + " reached the limit of bones, the least influencing bone discarded");

						BoneOrderNumber = 0;
						float MinWeight = (VertexFormat.BlendWeightSize != 8) ? Vertex.BlendWeights32[0] : ByteToNormalizedFloat((Vertex.BlendWeights8 >> (0 * 8)) & 0xff);
						for (size_t bo = 1; bo < MaxBonesPerVertex; ++bo)
						{
							const float Weight = (VertexFormat.BlendWeightSize != 8) ? Vertex.BlendWeights32[bo] : ByteToNormalizedFloat((Vertex.BlendWeights8 >> (bo * 8)) & 0xff);
							if (Weight < MinWeight)
							{
								BoneOrderNumber = bo;
								MinWeight = Weight;
							}
						}

						// The least influencing bone loses a vertex
						--FbxBones[Vertex.BlendIndices[BoneOrderNumber]].AffectedVertexCount;
					}
					else
					{
						BoneOrderNumber = Vertex.BonesUsed;

						++Vertex.BonesUsed;
						if (Vertex.BonesUsed > VertexFormat.BonesPerVertex)
							VertexFormat.BonesPerVertex = Vertex.BonesUsed;
					}

					Vertex.BlendIndices[BoneOrderNumber] = BoneIndex;
					if (VertexFormat.BlendWeightSize != 8)
						Vertex.BlendWeights32[BoneOrderNumber] = Weight;
					else
						reinterpret_cast<uint8_t*>(&Vertex.BlendWeights8)[3 - BoneOrderNumber] = NormalizedFloatToByte(Weight);

					++FbxBones[BoneIndex].AffectedVertexCount;
				}

				constexpr bool RenormalizeWeights = true;
				if (RenormalizeWeights)
					NormalizeWeights32x4(Vertex.BlendWeights32[0], Vertex.BlendWeights32[1], Vertex.BlendWeights32[2], Vertex.BlendWeights32[3]);

				Vertices.push_back(std::move(Vertex));
			}
		}

		// Check that all active bones influence at least one vertex
		// TODO: if validation failed, should implement recursive removal of unnecessary leaf bones, fixing vertex blend indices
		// NB: some vertices may be counted multiple times for now
		for (const auto& BoneInfo : FbxBones)
			assert(BoneInfo.WeightByControlPoint.empty() || BoneInfo.AffectedVertexCount > 0);

		std::swap(FbxBones, std::vector<CFbxBoneInfo>{});

		// Index and optimize vertices

		for (auto& [SubMeshID, SubMesh] : SubMeshes)
		{
			std::vector<CVertex> RawVertices;
			std::swap(RawVertices, SubMesh.Vertices);

			std::vector<unsigned int> RawIndices;

			ProcessGeometry(RawVertices, RawIndices, SubMesh.Vertices, SubMesh.Indices);

			if (!SubMesh.Vertices.empty())
			{
				InitAABBWithVertex(SubMesh.AABB, SubMesh.Vertices[0].Position);
				for (const auto& Vertex : SubMesh.Vertices)
					ExtendAABB(SubMesh.AABB, Vertex.Position);
			}
		}

		// TODO: simplify, quantize and compress if required, see meshoptimizer readme, can simplify for lower LODs

		// Write resulting mesh file

		std::string MeshName = pMesh->GetName();
		if (MeshName.empty()) MeshName = pMesh->GetNode()->GetName();
		if (MeshName.empty()) MeshName = Ctx.TaskName; //!!!FIXME: add counter per resource type!
		MeshName = GetValidResourceName(MeshName);

		const auto DestPath = Ctx.MeshPath / (MeshName + ".msh");

		if (!WriteDEMMesh(DestPath, SubMeshes, VertexFormat, Bones.size(), Ctx.Log)) return false;

		// Export materials, calculate AABB

		CMeshAttrInfo MeshInfo;
		MeshInfo.MeshID = _ResourceRoot + fs::relative(DestPath, _RootDir).generic_string();
		for (const auto& [SubMeshID, SubMesh] : SubMeshes)
		{
			fs::path MtlPath;
			if (!SubMeshID.empty())
			{
				// Search for the precreated material
				auto MtlIt = Ctx.MaterialMap.find(CStrID(SubMeshID.c_str()));
				if (MtlIt == Ctx.MaterialMap.cend())
					MtlIt = Ctx.MaterialMap.find(CStrID(GetValidResourceName(SubMeshID).c_str()));

				if (MtlIt == Ctx.MaterialMap.cend())
				{
					// Export a new material
					const int FbxMaterialIdx = pMesh->GetNode()->GetMaterialIndex(SubMeshID.c_str());
					if (FbxMaterialIdx >= 0 && ExportMaterial(pMesh->GetNode()->GetMaterial(FbxMaterialIdx), MtlPath, Ctx))
						Ctx.MaterialMap.emplace(CStrID(SubMeshID.c_str()), MtlPath.generic_string());
					else
						Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " material " + SubMeshID + " is not defined in FBX or .meta");
				}
				else MtlPath = MtlIt->second.GetValue<std::string>();
			}

			std::string MaterialID;
			if (!MtlPath.empty())
			{
				if (!_RootDir.empty() && MtlPath.is_absolute()) MtlPath = fs::relative(MtlPath, _RootDir);
				MaterialID = _ResourceRoot + MtlPath.generic_string();
			}
			MeshInfo.MaterialIDs.push_back(MaterialID);

			MergeAABBs(MeshInfo.AABB, SubMesh.AABB);
		}

		Ctx.ProcessedMeshes.emplace(pMesh, std::move(MeshInfo));

		// Write resulting skin file (if skinned)

		if (VertexFormat.BonesPerVertex)
		{
			const auto DestPath = Ctx.SkinPath / (MeshName + ".skn");
			if (!WriteDEMSkin(DestPath, Bones, Ctx.Log)) return false;

			// Calculate relative path from the skin node to the root joint

			std::string RootSearchPath;
			if (pMesh->GetNode() != pSkinRoot)
			{
				std::vector<std::string> CurrNodePath, SkeletonRootNodePath;
				GetAbsoluteFbxNodePath(pMesh->GetNode(), CurrNodePath);
				GetAbsoluteFbxNodePath(pSkinRoot, SkeletonRootNodePath);
				RootSearchPath = GetRelativeNodePath(std::move(CurrNodePath), std::move(SkeletonRootNodePath));
			}

			// Remember the skin for node attribute creation

			std::string SkinID = _ResourceRoot + fs::relative(DestPath, _RootDir).generic_string();
			Ctx.ProcessedSkins.emplace(pMesh, CSkinAttrInfo{ std::move(SkinID), std::move(RootSearchPath) });
		}

		return true;
	}

	void CollectTextures(FbxProperty& Property, std::vector<FbxFileTexture*>& Out)
	{
		int Count = Property.GetSrcObjectCount<FbxLayeredTexture>();
		for (int i = 0; i < Count; ++i)
		{
			FbxLayeredTexture* pLayeredTex = FbxCast<FbxLayeredTexture>(Property.GetSrcObject<FbxLayeredTexture>(i));
			const int LayerCount = pLayeredTex->GetSrcObjectCount<FbxTexture>();
			for (int j = 0; j < LayerCount; ++j)
				if (FbxFileTexture* pTex = FbxCast<FbxFileTexture>(pLayeredTex->GetSrcObject<FbxTexture>(j)))
					Out.push_back(pTex);
		}

		Count = Property.GetSrcObjectCount<FbxTexture>();
		for (int i = 0; i < Count; ++i)
			if (FbxFileTexture* pTex = FbxCast<FbxFileTexture>(Property.GetSrcObject<FbxTexture>(i)))
				Out.push_back(pTex);
	}

	// You can also declare existing DEM materials in .meta an associate them to FBX materials by name.
	// PBR materials aren't supported in FBX, can't export. Use glTF 2.0 exporter for PBR materials.
	// TODO: look at FBX SDK 2020.2 Standard Surface support.
	bool ExportMaterial(FbxSurfaceMaterial* pMaterial, fs::path& OutMaterialPath, CContext& Ctx)
	{
		if (!pMaterial) return false;

		const std::string MtlName = pMaterial->GetName();
		Ctx.Log.LogDebug("Exporting new material: " + MtlName);

		// Determine FBX surface type

		FbxSurfaceLambert* pLambert = FbxCast<FbxSurfaceLambert>(pMaterial);
		if (!pLambert)
		{
			Ctx.Log.LogWarning("Non-lambertian surfaces are not supported by cf-fbx");
			return false;
		}

		// Build abstract effect type ID

		//???TODO: detect AlphaTest if main diffuse texture has 1 bit alpha, Alpha if more bits?
		std::string EffectTypeID = "MetallicRoughnessOpaqueCulled";

		// Get effect resource ID and material table from the effect file

		auto EffectIt = _Settings.EffectsByType.find(EffectTypeID);
		if (EffectIt == _Settings.EffectsByType.cend() || EffectIt->second.empty())
		{
			Ctx.Log.LogError("FBX material " + MtlName + " with type " + EffectTypeID + " has no mapped DEM effect file in effect settings");
			return false;
		}

		CMaterialParams MtlParamTable;
		auto Path = ResolvePathAliases(EffectIt->second, _PathAliases).generic_string();
		Ctx.Log.LogDebug("Opening effect " + Path);
		if (!GetEffectMaterialParams(MtlParamTable, Path, Ctx.Log)) return false;

		Data::CParams MtlParams;

		// Fill material constants

		//!!!TODO: TransparentColor, TransparencyFactor!
		// Ambient & AmbientFactor are ignored
		const auto AlbedoFactorID = _Settings.GetEffectParamID("AlbedoFactor");
		if (MtlParamTable.HasConstant(AlbedoFactorID))
		{
			const auto& Value = FbxToDEMVec3(pLambert->Diffuse) * static_cast<float>(pLambert->DiffuseFactor);
			if (Value != float3(1.f, 1.f, 1.f))
				MtlParams.emplace_back(CStrID(AlbedoFactorID), float4(Value, 1.f));
		}

		const auto EmissiveFactorID = _Settings.GetEffectParamID("EmissiveFactor");
		if (MtlParamTable.HasConstant(EmissiveFactorID))
		{
			const auto& Value = FbxToDEMVec3(pLambert->Emissive) * static_cast<float>(pLambert->EmissiveFactor);
			if (Value != float3(0.f, 0.f, 0.f))
				MtlParams.emplace_back(CStrID(EmissiveFactorID), float4(Value, 0.f));
		}

		// Fill material textures and samplers
		// TODO: support layered textures?
		// TODO: support procedural textures?

		std::set<FbxTexture*> UsedTextures;

		const auto AlbedoTextureID = _Settings.GetEffectParamID("AlbedoTexture");
		if (MtlParamTable.HasResource(AlbedoTextureID))
		{
			std::vector<FbxFileTexture*> Textures;
			CollectTextures(pLambert->Diffuse, Textures);

			if (!Textures.empty())
			{
				// For now export only the first texture
				if (Textures.size() > 1)
					Ctx.Log.LogWarning("There are more than one albedo texture in a material " + MtlName + ", only one will be used");

				std::string TextureID;
				if (!ExportTexture(*Textures.begin(), TextureID, Ctx)) return false;
				UsedTextures.insert(*Textures.begin());
				MtlParams.emplace_back(AlbedoTextureID, TextureID);
			}
		}

		const auto NormalTextureID = _Settings.GetEffectParamID("NormalTexture");
		if (MtlParamTable.HasResource(NormalTextureID))
		{
			std::vector<FbxFileTexture*> Textures;
			CollectTextures(pLambert->NormalMap, Textures);
			CollectTextures(pLambert->Bump, Textures);

			if (!Textures.empty())
			{
				// For now export only the first texture
				if (Textures.size() > 1)
					Ctx.Log.LogWarning("There are more than one normal texture in a material " + MtlName + ", only one will be used");

				std::string TextureID;
				if (!ExportTexture(*Textures.begin(), TextureID, Ctx)) return false;
				UsedTextures.insert(*Textures.begin());
				MtlParams.emplace_back(NormalTextureID, TextureID);
			}
		}

		const auto EmissiveTextureID = _Settings.GetEffectParamID("EmissiveTexture");
		if (MtlParamTable.HasResource(EmissiveTextureID))
		{
			std::vector<FbxFileTexture*> Textures;
			CollectTextures(pLambert->NormalMap, Textures);
			CollectTextures(pLambert->Bump, Textures);

			if (!Textures.empty())
			{
				// For now export only the first texture
				if (Textures.size() > 1)
					Ctx.Log.LogWarning("There are more than one emissive texture in a material " + MtlName + ", only one will be used");

				std::string TextureID;
				if (!ExportTexture(*Textures.begin(), TextureID, Ctx)) return false;
				UsedTextures.insert(*Textures.begin());
				MtlParams.emplace_back(EmissiveTextureID, TextureID);
			}
		}

		// For phong surfaces convert specular to roughness. For non-phong an effect should provide
		// default roughness factor or texture. We don't try to set defaults here.
		if (FbxSurfacePhong* pPhong = FbxCast<FbxSurfacePhong>(pMaterial))
		{
			// Convert Blinn-Phong specular power (shininess) into PBR roughness
			// See http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html
			const float Roughness = std::sqrtf(2.f / (static_cast<float>(pPhong->Shininess) + 2.f));

			const auto RoughnessFactorID = _Settings.GetEffectParamID("RoughnessFactor");
			if (Roughness != 1.f && MtlParamTable.HasConstant(RoughnessFactorID))
				MtlParams.emplace_back(RoughnessFactorID, Roughness);

			const auto MetallicRoughnessTextureID = _Settings.GetEffectParamID("MetallicRoughnessTexture");
			if (MtlParamTable.HasResource(MetallicRoughnessTextureID))
			{
				// TODO: can create roughness texture from Specular (tex) * SpecularFactor and using shininess!
				// Make specular monochrome! Use fixed metallness (zero, phong is a plastic)!

				//std::string TextureID;
				//if (!ExportTexture(Mtl.metallicRoughness.metallicRoughnessTexture, TextureID, Ctx)) return false;
				//UsedTextures.insert(*Textures.begin());
				//MtlParams.emplace_back(MetallicRoughnessTextureID, TextureID);
			}
		}

		if (!UsedTextures.empty())
		{
			const FbxTexture* pTex = *UsedTextures.cbegin();

			bool NotEqual = false;
			for (auto It = ++UsedTextures.cbegin(); It != UsedTextures.cend(); ++It)
			{
				if (pTex->GetWrapModeU() != (*It)->GetWrapModeU() || pTex->GetWrapModeV() != (*It)->GetWrapModeV())
				{
					NotEqual = true;
					break;
				}
			}

			if (NotEqual)
				Ctx.Log.LogWarning("Material " + MtlName + " uses more than one sampler, but DEM supports only one sampler per PBR material");

			// NB: FBX doesn't support texture filtering
			const auto PBRTextureSamplerID = _Settings.GetEffectParamID("PBRTextureSampler");
			if (MtlParamTable.HasSampler(PBRTextureSamplerID))
			{
				Data::CParams SamplerDesc;

				switch (pTex->GetWrapModeU())
				{
					case FbxTexture::EWrapMode::eRepeat: SamplerDesc.emplace_back(CStrID("AddressU"), std::string("wrap")); break;
					case FbxTexture::EWrapMode::eClamp: SamplerDesc.emplace_back(CStrID("AddressU"), std::string("clamp")); break;
				}

				switch (pTex->GetWrapModeV())
				{
					case FbxTexture::EWrapMode::eRepeat: SamplerDesc.emplace_back(CStrID("AddressV"), std::string("wrap")); break;
					case FbxTexture::EWrapMode::eClamp: SamplerDesc.emplace_back(CStrID("AddressV"), std::string("clamp")); break;
				}

				if (!SamplerDesc.empty()) MtlParams.emplace_back(PBRTextureSamplerID, std::move(SamplerDesc));
			}
		}

		// Write resulting file

		const auto DestPath = Ctx.MaterialPath / (GetValidResourceName(MtlName) + ".mtl");
		fs::create_directories(DestPath.parent_path());
		std::ofstream File(DestPath, std::ios_base::binary | std::ios_base::trunc);
		if (!SaveMaterial(File, EffectIt->second, MtlParamTable, MtlParams, Ctx.Log)) return false;

		OutMaterialPath = _RootDir.empty() ? DestPath : fs::relative(DestPath, _RootDir);

		return true;
	}

	bool ExportTexture(FbxFileTexture* pTex, std::string& OutTextureID, CContext& Ctx)
	{
		auto It = Ctx.ProcessedTextures.find(pTex);
		if (It != Ctx.ProcessedTextures.cend())
		{
			OutTextureID = It->second;
			return true;
		}

		fs::path URI = pTex->GetFileName();
		Ctx.Log.LogDebug("Texture image: " + URI.generic_string() + (pTex->GetName() ? "" : std::string(", name: ") + pTex->GetName()));

		const auto DestPath = WriteTexture(URI, Ctx.TexturePath, Ctx.TaskParams, Ctx.Log);
		if (DestPath.empty()) return false;

		OutTextureID = _ResourceRoot + fs::relative(DestPath, _RootDir).generic_string();
		Ctx.ProcessedTextures.emplace(pTex, OutTextureID);

		return true;
	}

	bool ExportLight(FbxLight* pLight, CContext& Ctx, Data::CDataArray& Attributes)
	{
		Ctx.Log.LogDebug("Light");

		int LightClassFourCC = 0;
		switch (pLight->LightType.Get())
		{
			case FbxLight::eDirectional: LightClassFourCC = 'DLTA'; break;
			case FbxLight::ePoint: LightClassFourCC = 'PLTA'; break;
			case FbxLight::eSpot: LightClassFourCC = 'SLTA'; break;
			default:
			{
				Ctx.Log.LogWarning(std::string("Light ") + pLight->GetName() + " is of unsupported type, skipped");
				return true;
			}
		}

		const float3 Color = FbxToDEMVec3(pLight->Color.Get());
		const float Intensity = static_cast<float>(pLight->Intensity.Get() / 100.0);

		float Range = 0.f;
		if (LightClassFourCC != 'DLTA')
		{
			if (pLight->EnableFarAttenuation.Get())
			{
				Range = static_cast<float>(pLight->FarAttenuationEnd.Get());
			}
			else
			{
				// When intensity lowers so that it can't change a pixel color, we consider the light completely decayed
				constexpr float MinIntensity = /*0.5f **/ (1.f / 256.f);

				const float DistanceFactor = Intensity / MinIntensity;

				switch (pLight->DecayType.Get())
				{
					case FbxLight::eLinear: Range = DistanceFactor; break;
					case FbxLight::eQuadratic: Range = std::sqrtf(DistanceFactor); break;
					case FbxLight::eCubic: Range = std::powf(DistanceFactor, 1.f / 3.f); break;
					default:
					{
						Ctx.Log.LogWarning(std::string("Light ") + pLight->GetName() + " has unsupported decay type, skipped");
						return true;
					}
				}
			}
		}

		Data::CParams Attribute;
		Attribute.emplace_back(CStrID("Class"), LightClassFourCC);
		Attribute.emplace_back(CStrID("Color"), (int)ColorRGBANorm(Color.x, Color.y, Color.z));
		Attribute.emplace_back(CStrID("Intensity"), Intensity);

		if (pLight->CastShadows.Get())
			Attribute.emplace_back(CStrID("CastShadows"), true);

		if (LightClassFourCC != 'DLTA')
		{
			Attribute.emplace_back(CStrID("Range"), Range);

			if (LightClassFourCC == 'SLTA')
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
		Attribute.emplace_back(CStrID("Class"), 'CAMR'); //std::string("Frame::CCameraAttribute"));

		if (pCamera->ProjectionType.Get() == FbxCamera::eOrthogonal)
			Attribute.emplace_back(CStrID("Orthogonal"), true);

		// FIXME: not all of subsequent params are needed for orthographic cameras!

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
			Thresholds[i].second = pChild ? GetValidNodeName(pChild->GetName()) : std::string();
		}

		const FbxNode* pLastChild = pLODGroup->GetNode()->GetChild(pLODGroup->GetNode()->GetChildCount());
		std::string LastID = pLastChild ? GetValidNodeName(pLastChild->GetName()) : std::string();

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
		Attribute.emplace_back(CStrID("Class"), 'LODG'); //std::string("Scene::CLODGroup"));

		//!!!can't store float -> string in a Data::CParams! Use array of sections with 2 fields each?
		//!!!store the last one! //???use FLT_MAX for uniformity of the list?

		assert(false && "IMPLEMENT ME!!!");

		Attributes.push_back(std::move(Attribute));

		return true;
	}

	bool ExportAnimation(FbxAnimStack* pAnimStack, FbxScene* pScene, CContext& Ctx)
	{
		// Strip node name from the animation clip name
		std::string AnimName = pAnimStack->GetName();
		const auto DlmPos = AnimName.find('|');
		if (DlmPos != std::string::npos)
			AnimName = AnimName.substr(DlmPos + 1);

		AnimName = GetValidResourceName(AnimName);

		if (Ctx.ScenePath.empty() && pScene->GetSrcObjectCount<FbxAnimStack>() == 1 && (AnimName.empty() || AnimName == "mixamo_com"))
			AnimName = Ctx.TaskName;

		const auto LayerCount = pAnimStack->GetMemberCount<FbxAnimLayer>();
		if (LayerCount <= 0)
		{
			Ctx.Log.LogWarning("Animation " + AnimName + " has no layers, skipped");
			return true;
		}
		else if (LayerCount > 1)
		{
			Ctx.Log.LogWarning("Animation " + AnimName + " has more than one layer");
		}

		const FbxTakeInfo* pTakeInfo = pScene->GetTakeInfo(pAnimStack->GetName());
		const FbxLongLong StartFrame = pTakeInfo->mLocalTimeSpan.GetStart().GetFrameCount(FbxTime::GetGlobalTimeMode());
		const FbxLongLong EndFrame = pTakeInfo->mLocalTimeSpan.GetStop().GetFrameCount(FbxTime::GetGlobalTimeMode());

		Ctx.Log.LogDebug("Animation " + AnimName +
			", frames " + std::to_string(StartFrame) + '-' + std::to_string(EndFrame));

		const auto pLayer = pAnimStack->GetMember<FbxAnimLayer>(0);
		if (!pLayer || !pLayer->GetMemberCount())
		{
			Ctx.Log.LogWarning("Animation " + AnimName + " is empty, skipped");
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
			Ctx.Log.LogWarning("Animation " + AnimName + " doesn't affect any node transformation, skipped");
			return true;
		}

		// Read animation clip settings

		bool DiscardRootMotion = false;
		bool IsLocomotionClip = false;
		const Data::CParams* pParams = nullptr;
		if (ParamsUtils::TryGetParam(pParams, Ctx.TaskParams, "Animations"))
		{
			const Data::CParams* pClipParams = nullptr;
			if (ParamsUtils::TryGetParam(pClipParams, *pParams, AnimName.c_str()))
			{
				DiscardRootMotion = ParamsUtils::GetParam(*pClipParams, "DiscardRootMotion", false);
				IsLocomotionClip = ParamsUtils::GetParam(*pClipParams, "IsLocomotionClip", false);
			}
		}

		// Create rigid skeletons and associate ACL bones with FbxNode instances

		std::vector<CSkeletonACLBinding> Skeletons;
		for (FbxNode* pRoot : SkeletonRoots)
		{
			CSkeletonACLBinding Skeleton;

			std::vector<acl::RigidBone> Bones;
			BuildACLSkeleton(pAnimStack, pRoot, Skeleton.Nodes, Bones);

			assert(Bones.size() <= std::numeric_limits<uint16_t>().max());

			Skeleton.Skeleton = acl::make_unique<acl::RigidSkeleton>(
				Ctx.ACLAllocator, Ctx.ACLAllocator, Bones.data(), static_cast<uint16_t>(Bones.size()));

			Skeletons.push_back(std::move(Skeleton));
		}

		// Evaluate animation of all skeletons frame by frame and compress with ACL

		pScene->SetCurrentAnimationStack(pAnimStack);

		const auto FrameRate = static_cast<float>(FbxTime::GetFrameRate(FbxTime::GetGlobalTimeMode()));
		const auto FrameCount = static_cast<uint32_t>(EndFrame - StartFrame + 1);

		// FIXME: saving clip for each skeleton separately will erase previously saved .anm file
		assert(Skeletons.size() == 1);

		for (CSkeletonACLBinding& Skeleton : Skeletons)
		{
			auto& Nodes = Skeleton.Nodes;

			acl::String ClipName(Ctx.ACLAllocator, AnimName.c_str());
			acl::AnimationClip Clip(Ctx.ACLAllocator, *Skeleton.Skeleton, FrameCount, FrameRate, ClipName);

			std::vector<std::string> NodeNames(Skeleton.Nodes.size());
			for (size_t BoneIdx = 0; BoneIdx < Skeleton.Nodes.size(); ++BoneIdx)
				NodeNames[BoneIdx] = GetValidNodeName(Skeleton.Nodes[BoneIdx]->GetName());

			FbxTime FrameTime;
			constexpr size_t RootIdx = 0;
			std::vector<acl::Vector4_32> RootPositions(FrameCount);

			// Process foot bones for locomotion phase matching
			size_t LeftFootIdx = std::numeric_limits<size_t>().max();
			size_t RightFootIdx = std::numeric_limits<size_t>().max();
			std::vector<acl::Vector4_32> LeftFootPositions;
			std::vector<acl::Vector4_32> RightFootPositions;
			if (IsLocomotionClip && !Ctx.LeftFootBoneName.empty() && !Ctx.RightFootBoneName.empty())
			{
				auto ItLeft = std::find_if(Nodes.begin(), Nodes.end(), [&Ctx](FbxNode* pNode) { return Ctx.LeftFootBoneName == pNode->GetName(); });
				auto ItRight = std::find_if(Nodes.begin(), Nodes.end(), [&Ctx](FbxNode* pNode) { return Ctx.RightFootBoneName == pNode->GetName(); });
				if (ItLeft != Nodes.end() && ItRight != Nodes.end())
				{
					LeftFootIdx = std::distance(Nodes.begin(), ItLeft);
					RightFootIdx = std::distance(Nodes.begin(), ItRight);
					LeftFootPositions.resize(FrameCount);
					RightFootPositions.resize(FrameCount);
				}
				else
				{
					IsLocomotionClip = false;
					Ctx.Log.LogWarning("Could not find leg bones in a locomotion clip. Locomotion flag is discarded.");
				}
			}

			for (size_t BoneIdx = 0; BoneIdx < Nodes.size(); ++BoneIdx)
			{
				acl::AnimatedBone& Bone = Clip.get_animated_bone(static_cast<uint16_t>(BoneIdx));
				FbxNode* pNode = Nodes[BoneIdx];

				uint32_t SampleIndex = 0;
				for (FbxLongLong Frame = StartFrame; Frame <= EndFrame; ++Frame, ++SampleIndex)
				{
					FrameTime.SetFrame(Frame, FbxTime::GetGlobalTimeMode());

					// Collect global positions of special bones
					std::vector<acl::Vector4_32>* pPositions =
						(BoneIdx == LeftFootIdx) ? &LeftFootPositions :
						(BoneIdx == RightFootIdx) ? &RightFootPositions :
						(BoneIdx == RootIdx) ? &RootPositions :
						nullptr;

					if (pPositions)
					{
						const auto GlobalTfm = pNode->EvaluateGlobalTransform(FrameTime);
						const auto GT = GlobalTfm.GetT();
						(*pPositions)[SampleIndex] = { static_cast<float>(GT[0]), static_cast<float>(GT[1]), static_cast<float>(GT[2]), 1.0f };
					}

					if (BoneIdx == RootIdx && DiscardRootMotion && Frame > StartFrame)
					{
						// All root transforms will be as in the first frame
						//???Add options do discard only some components, like translation XZ?
						Bone.scale_track.set_sample(SampleIndex, Bone.scale_track.get_sample(0));
						Bone.rotation_track.set_sample(SampleIndex, Bone.rotation_track.get_sample(0));
						Bone.translation_track.set_sample(SampleIndex, Bone.translation_track.get_sample(0));
					}
					else
					{
						const auto LocalTfm = pNode->EvaluateLocalTransform(FrameTime);
						const auto S = LocalTfm.GetS();
						const auto R = LocalTfm.GetQ();
						const auto T = LocalTfm.GetT();

						Bone.scale_track.set_sample(SampleIndex, { S[0], S[1], S[2], 1.0 });
						Bone.rotation_track.set_sample(SampleIndex, { R[0], R[1], R[2], R[3] });
						Bone.translation_track.set_sample(SampleIndex, { T[0], T[1], T[2], 1.0 });
					}
				}
			}

			CLocomotionInfo LocomotionInfo;
			if (IsLocomotionClip)
			{
				FbxAMatrix GlobalTfm;
				auto& AxisSys = pScene->GetGlobalSettings().GetAxisSystem();
				AxisSys.GetMatrix(GlobalTfm);

				const auto Fwd = GlobalTfm.GetColumn(2);
				acl::Vector4_32 ForwardDir = { static_cast<float>(Fwd[0]), static_cast<float>(Fwd[1]), static_cast<float>(Fwd[2]), 0.0f };

				const auto Up = GlobalTfm.GetColumn(1);
				acl::Vector4_32 UpDir = { static_cast<float>(Up[0]), static_cast<float>(Up[1]), static_cast<float>(Up[2]), 0.0f };

				const auto Side = GlobalTfm.GetColumn(0);
				acl::Vector4_32 SideDir = { static_cast<float>(Side[0]), static_cast<float>(Side[1]), static_cast<float>(Side[2]), 0.0f };

				ComputeLocomotion(LocomotionInfo, FrameRate, ForwardDir, UpDir, SideDir, RootPositions, LeftFootPositions, RightFootPositions);
			}

			const auto DestPath = Ctx.AnimPath / (AnimName + ".anm");
			if (!WriteDEMAnimation(DestPath, Ctx.ACLAllocator, Clip, NodeNames, IsLocomotionClip ? &LocomotionInfo : nullptr, Ctx.Log)) return false;
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

	// Returns number of animated nodes in this subtree
	size_t BuildACLSkeleton(FbxAnimStack* pAnimStack, FbxNode* pNode, std::vector<FbxNode*>& Nodes, std::vector<acl::RigidBone>& Bones)
	{
		size_t AnimatedCount = 0;
		if (IsPropertyAnimated(pAnimStack, pNode->LclScaling) ||
			IsPropertyAnimated(pAnimStack, pNode->LclRotation) ||
			IsPropertyAnimated(pAnimStack, pNode->LclTranslation))
		{
			++AnimatedCount;
		}

		acl::RigidBone Bone;
		if (Nodes.empty())
		{
			Bone.parent_index = acl::k_invalid_bone_index;
		}
		else
		{
			auto It = std::find(Nodes.crbegin(), Nodes.crend(), pNode->GetParent());
			assert(It != Nodes.crend());
			Bone.parent_index = static_cast<uint16_t>(std::distance(Nodes.cbegin(), It.base()) - 1);
		}

		// TODO: metric from per-bone AABBs?
		Bone.vertex_distance = 3.f;

		Nodes.push_back(pNode);
		Bones.push_back(std::move(Bone));

		for (int i = 0; i < pNode->GetChildCount(); ++i)
		{
			AnimatedCount += BuildACLSkeleton(pAnimStack, pNode->GetChild(i), Nodes, Bones);
			/*
			// This was faster than checking IsPropertyAnimated and using AnimatedCount.
			// Disabled because some models do not have FbxNodeAttribute::eSkeleton on animated nodes, see for example:
			// https://www.turbosquid.com/FullPreview/Index.cfm/ID/1049650

			// Traverse only through bones
			for (int i = 0; i < pNode->GetNodeAttributeCount(); ++i)
			{
				if (pNode->GetNodeAttributeByIndex(i)->GetAttributeType() == FbxNodeAttribute::eSkeleton)
				{
					BuildACLSkeleton(pNode->GetChild(i), Nodes, Bones);
					break;
				}
			}
			*/
		}

		if (!AnimatedCount)
		{
			// Subtree isn't animated by this clip, discard the node
			Nodes.pop_back();
			Bones.pop_back();
		}

		return AnimatedCount;
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
