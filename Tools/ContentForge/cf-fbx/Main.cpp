#include <ContentForgeTool.h>
#include <Render/RenderEnums.h>
#include <Utils.h>
#include <fbxsdk.h>
#include <meshoptimizer.h>
//#include <CLI11.hpp>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/scenes

constexpr size_t MaxUV = 4;
constexpr size_t MaxBonesPerVertex = 4;

static void ConvertTransformsToSRTRecursive(FbxNode* pNode)
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

	// DEM is capable of slerp
	pNode->SetQuaternionInterpolation(FbxNode::eDestinationPivot, EFbxQuatInterpMode::eQuatInterpSlerp);

	for (int i = 0; i < pNode->GetChildCount(); ++i)
		ConvertTransformsToSRTRecursive(pNode->GetChild(i));
}

template<typename TOut, typename TElement>
static void GetVertexElement(TOut& OutValue, TElement* pElement, int ControlPointIndex, int VertexIndex)
{
	if (!pElement) return;

	int ID;
	switch (pElement->GetMappingMode())
	{
		case FbxGeometryElement::eByControlPoint: ID = ControlPointIndex; break;
		case FbxGeometryElement::eByPolygonVertex: ID = VertexIndex; break;
		case FbxGeometryElement::eByPolygon: ID = VertexIndex / 3; break;
		case FbxGeometryElement::eAllSame: ID = 0; break;
		default: return;
	}

	if (pElement->GetReferenceMode() != FbxGeometryElement::eDirect)
		ID = pElement->GetIndexArray().GetAt(ID);

	OutValue = pElement->GetDirectArray().GetAt(ID);
}

static void WriteVertexComponent(std::ostream& Stream, EVertexComponentSemantic Semantic, EVertexComponentFormat Format, uint8_t Index, uint8_t StreamIndex)
{
	WriteStream(Stream, static_cast<uint8_t>(Semantic));
	WriteStream(Stream, static_cast<uint8_t>(Format));
	WriteStream(Stream, Index);
	WriteStream(Stream, StreamIndex);
}

class CFBXTool : public CContentForgeTool
{
protected:

	struct CContext
	{
		CThreadSafeLog& Log;
		fs::path        MeshPath;
		fs::path        SkinPath;
		std::string     DefaultName;
	};

	//???TODO: floats? convert on read from FbxMesh?
	struct CVertex
	{
		int        ControlPointIndex;
		float      Position[3];
		FbxDouble3 Normal;
		FbxDouble3 Tangent;
		FbxDouble3 Bitangent;
		FbxColor   Color;
		FbxVector2 UV[MaxUV];
		int        BlendIndices[MaxBonesPerVertex];
		float      BlendWeights[MaxBonesPerVertex];
		size_t     BonesUsed = 0;
	};

	struct CBone
	{
		const FbxNode* pBone;
		FbxAMatrix     InvLocalBindPose;
	};

	FbxManager* pFBXManager = nullptr;

public:

	CFBXTool(const std::string& Name, const std::string& Desc, CVersion Version) :
		CContentForgeTool(Name, Desc, Version)
	{
		// Set default before parsing command line
		_RootDir = "../../../content";
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

		return 0;
	}

	virtual int Term() override
	{
		pFBXManager->Destroy();
		pFBXManager = nullptr;
		return 0;
	}

	//virtual void ProcessCommandLine(CLI::App& CLIApp) override
	//{
	//	CContentForgeTool::ProcessCommandLine(CLIApp);
	//}

	virtual bool ProcessTask(CContentForgeTask& Task) override
	{
		// TODO: check whether the metafile can be processed by this tool

		constexpr double AnimSamplingRate = 30.0;

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
			Task.Log.LogDebug("Source format: FBX v" + std::to_string(Major) + '.' + std::to_string(Minor) + '.' + std::to_string(Revision));
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
		// TODO: save results back to FBX?

		ConvertTransformsToSRTRecursive(pScene->GetRootNode());
		pScene->GetRootNode()->ConvertPivotAnimationRecursive(nullptr, FbxNode::eDestinationPivot, AnimSamplingRate);

		{
			FbxGeometryConverter GeometryConverter(pFBXManager);

			if (!GeometryConverter.Triangulate(pScene, true))
				Task.Log.LogWarning("Couldn't triangulate some geometry");

			GeometryConverter.RemoveBadPolygonsFromMeshes(pScene);

			if (!GeometryConverter.SplitMeshesPerMaterial(pScene, true))
				Task.Log.LogWarning("Couldn't split some meshes per material");
		}

		// Prepare task context

		//!!!TODO: need flags, what to export! command-line override must be provided along with .meta params
		CContext Ctx{ Task.Log };

		Ctx.DefaultName = Task.TaskID.CStr();

		Ctx.MeshPath = GetParam<std::string>(Task.Params, "MeshOutput", std::string{});
		if (!_RootDir.empty() && Ctx.MeshPath.is_relative())
			Ctx.MeshPath = fs::path(_RootDir) / Ctx.MeshPath;

		Ctx.SkinPath = GetParam<std::string>(Task.Params, "SkinOutput", std::string{});
		if (!_RootDir.empty() && Ctx.SkinPath.is_relative())
			Ctx.SkinPath = fs::path(_RootDir) / Ctx.SkinPath;

		// Export node hierarchy to DEM format

		if (!ExportNode(pScene->GetRootNode(), Ctx)) return false;

		// Export animations

		// ...

		return true;
	}

	bool ExportNode(FbxNode* pNode, CContext& Ctx)
	{
		if (!pNode)
		{
			Ctx.Log.LogWarning("Empty FBX node encountered");
			return true;
		}

		// Process node info

		const char* pName = pNode->GetName();
		FbxDouble3 Translation = pNode->LclTranslation.Get();
		FbxDouble3 Scaling = pNode->LclScaling.Get();
		FbxQuaternion Rotation;
		Rotation.ComposeSphericalXYZ(pNode->LclRotation.Get());

		Ctx.Log.LogDebug("Node");

		// Process attributes

		for (int i = 0; i < pNode->GetNodeAttributeCount(); ++i)
		{
			auto pAttribute = pNode->GetNodeAttributeByIndex(i);

			switch (pAttribute->GetAttributeType())
			{
				case FbxNodeAttribute::eMesh:
				{
					if (!ExportModel(static_cast<FbxMesh*>(pAttribute), Ctx)) return false;
					break;
				}
				case FbxNodeAttribute::eLight:
				{
					if (!ExportLight(static_cast<FbxLight*>(pAttribute), Ctx)) return false;
					break;
				}
				case FbxNodeAttribute::eCamera:
				{
					if (!ExportCamera(static_cast<FbxCamera*>(pAttribute), Ctx)) return false;
					break;
				}
				case FbxNodeAttribute::eLODGroup:
				{
					if (!ExportLODGroup(static_cast<FbxLODGroup*>(pAttribute), Ctx)) return false;
					break;
				}
				/*
				eSkeleton, 
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

		// Process children

		for (int i = 0; i < pNode->GetChildCount(); ++i)
			if (!ExportNode(pNode->GetChild(i), Ctx)) return false;

		return true;
	}

	bool ExportModel(const FbxMesh* pMesh, CContext& Ctx)
	{
		Ctx.Log.LogDebug("Model");

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

		// Collect vertices

		const int PolyCount = pMesh->GetPolygonCount();

		std::vector<CVertex> RawVertices;
		RawVertices.reserve(static_cast<size_t>(PolyCount * 3));

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

			// Process polygon vertices

			for (int v = 0; v < PolySize; ++v)
			{
				const auto VertexIndex = static_cast<unsigned int>(RawVertices.size());
				const auto ControlPointIndex = pMesh->GetPolygonVertex(p, v);

				const auto ControlPoint = pControlPoints[ControlPointIndex];

				CVertex Vertex{ 0 };
				Vertex.ControlPointIndex = ControlPointIndex;

				// We need float3 positions for meshopt_optimizeOverdraw
				Vertex.Position[0] = static_cast<float>(ControlPoint[0]);
				Vertex.Position[1] = static_cast<float>(ControlPoint[1]);
				Vertex.Position[2] = static_cast<float>(ControlPoint[2]);

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

				RawVertices.push_back(std::move(Vertex));
			}
		}

		// Index and optimize vertices

		std::vector<unsigned int> Indices(RawVertices.size());
		const auto VertexCount = meshopt_generateVertexRemap(Indices.data(), nullptr, RawVertices.size(), RawVertices.data(), RawVertices.size(), sizeof(CVertex));

		std::vector<CVertex> Vertices(VertexCount);
		meshopt_remapVertexBuffer(Vertices.data(), RawVertices.data(), RawVertices.size(), sizeof(CVertex), Indices.data());

		// NB: meshopt_remapIndexBuffer is not needed, as we have no source indices,
		//     and remap array is effectively an index array
		//std::vector<unsigned int> Indices2(Indices.size());
		//meshopt_remapIndexBuffer(Indices2.data(), nullptr, Indices.size(), Indices.data());

		meshopt_optimizeVertexCache(Indices.data(), Indices.data(), Indices.size(), Vertices.size());

		meshopt_optimizeOverdraw(Indices.data(), Indices.data(), Indices.size(), &Vertices[0].Position[0], Vertices.size(), sizeof(CVertex), 1.05f);

		meshopt_optimizeVertexFetch(Vertices.data(), Indices.data(), Indices.size(), Vertices.data(), Vertices.size(), sizeof(CVertex));

		// TODO: meshopt_generateShadowIndexBuffer for Z prepass and shadow rendering.
		// Also can separate positions from all other data into 2 vertex streams, and use only positions for shadows & Z prepass.

		// Process skin
		// NB: skin is per-control-point, so it is better done after optimizing out redundant vertices

		std::vector<CBone> Bones;
		size_t MaxBonesPerVertexUsed = 0;

		const FbxAMatrix InvMeshWorldMatrix = pMesh->GetNode()->EvaluateGlobalTransform().Inverse();

		const auto SkinCount = pMesh->GetDeformerCount(FbxDeformer::eSkin);
		for (int s = 0; s < SkinCount; ++s)
		{
			const FbxSkin* pSkin = static_cast<FbxSkin*>(pMesh->GetDeformer(s, FbxDeformer::eSkin));
			const auto ClusterCount = pSkin->GetClusterCount();

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
					if (std::fabsf(Weight) <= std::numeric_limits<float>().epsilon()) continue;

					for (auto& Vertex : Vertices)
					{
						if (Vertex.ControlPointIndex != ControlPointIndex) continue;

						if (Vertex.BonesUsed >= MaxBonesPerVertex)
						{
							Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " control point " + std::to_string(ControlPointIndex) + " reached the limit of influencing bones, the rest is discarded");
							continue;
						}

						++VerticesAffected;

						Vertex.BlendIndices[Vertex.BonesUsed] = BoneIndex;
						Vertex.BlendWeights[Vertex.BonesUsed] = Weight;

						++Vertex.BonesUsed;
						if (Vertex.BonesUsed > MaxBonesPerVertexUsed)
							MaxBonesPerVertexUsed = Vertex.BonesUsed;
					}
				}

				if (VerticesAffected)
				{
					FbxAMatrix WorldBindPose;
					pCluster->GetTransformLinkMatrix(WorldBindPose);

					Bones.push_back(CBone{ pBone, (InvMeshWorldMatrix * WorldBindPose).Inverse() });
				}
			}
		}

		// TODO: simplify, quantize and compress if required, see meshoptimizer readme, can simplify for lower LODs

		// Write resulting mesh file

		fs::create_directories(Ctx.MeshPath);

		std::string MeshName = pMesh->GetName();
		if (MeshName.empty()) MeshName = pMesh->GetNode()->GetName();
		if (MeshName.empty()) MeshName = Ctx.DefaultName; //!!!FIXME: add counter per resource type!
		ToLower(MeshName);

		// TODO: replace forbidden characters (std::transform with replacer callback?)

		const auto DestPath = Ctx.MeshPath / (MeshName + ".msh");

		std::ofstream File(DestPath, std::ios_base::binary);
		if (!File)
		{
			Ctx.Log.LogError("Error opening an output file " + DestPath.string());
			return false;
		}

		WriteStream<uint32_t>(File, 'MESH');        // Format magic value
		WriteStream<uint32_t>(File, 0x00010000);    // Version 0.1.0.0
		WriteStream<uint32_t>(File, 1);             // Group count, now always 1, may change later!

		WriteStream(File, static_cast<uint32_t>(Vertices.size()));
		WriteStream(File, static_cast<uint32_t>(Indices.size()));

		// One index size in bytes
		const bool Indices32 = (Vertices.size() > std::numeric_limits<uint16_t>().max());
		if (Indices32)
		{
			Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " has " + std::to_string(Vertices.size()) + " vertices and will use 32-bit indices");
			static_assert(sizeof(unsigned int) == 4);
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
				WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneIndices, EVertexComponentFormat::VCFmt_SInt16_4, 0, 0);
			else
				WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneIndices, EVertexComponentFormat::VCFmt_UInt8_4, 0, 0);

			if (MaxBonesPerVertexUsed == 1)
				WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneWeights, EVertexComponentFormat::VCFmt_Float32_1, 0, 0);
			else if (MaxBonesPerVertexUsed == 2)
				WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneWeights, EVertexComponentFormat::VCFmt_Float32_2, 0, 0);
			else if (MaxBonesPerVertexUsed == 3)
				WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneWeights, EVertexComponentFormat::VCFmt_Float32_3, 0, 0);
			else if (MaxBonesPerVertexUsed == 4)
				WriteVertexComponent(File, EVertexComponentSemantic::VCSem_BoneWeights, EVertexComponentFormat::VCFmt_Float32_4, 0, 0);
		}

		// Save mesh groups (always 1 group x 1 LOD now, may change later)
		WriteStream<uint32_t>(File, 0);                            // First vertex
		WriteStream(File, static_cast<uint32_t>(Vertices.size())); // Vertex count
		WriteStream<uint32_t>(File, 0);                            // First index
		WriteStream(File, static_cast<uint32_t>(Indices.size()));  // Index count
		WriteStream(File, static_cast<uint8_t>(EPrimitiveTopology::Prim_TriList));

		// TODO: precalculate group AABB!

		// TODO: save padding size and padding for vertices and indices, so that memory-mapped file
		// will have vertex and index arrays aligned! instead of padding may save offsets to data,
		// ensure they are aligned-16.

		// Save vertices (write one by one, component by component)

		// Save indices

		// Save skin palette (separate file? or no reason to split? two resources in one file?)
		//???or mesh resource stores skin palette resource pointer?

		// Material

		if (pMesh->GetElementMaterialCount())
		{
			// We splitted meshes per material at the start
			assert(pMesh->GetElementMaterialCount() < 2);

			auto pMaterial = pMesh->GetElementMaterial(0);

			// - custom constants?
			// - texture pathes (PBR through layered texture? or custom channels?)
			//???how to process animated materials?
		}
		else
		{
			//???use some custom property to set external material by resource ID?
			Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " has no material attached");
		}

		return true;
	}

	bool ExportLight(FbxLight* pLight, CContext& Ctx)
	{
		Ctx.Log.LogDebug("Light");

		return true;
	}

	bool ExportCamera(FbxCamera* pCamera, CContext& Ctx)
	{
		Ctx.Log.LogDebug("Camera");

		return true;
	}

	bool ExportLODGroup(FbxLODGroup* pLODGroup, CContext& Ctx)
	{
		Ctx.Log.LogDebug("LOD group");

		return true;
	}
};

int main(int argc, const char** argv)
{
	CFBXTool Tool("cf-fbx", "FBX to DeusExMachina resource converter", { 0, 1, 0 });
	return Tool.Execute(argc, argv);
}
