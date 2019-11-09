#include <ContentForgeTool.h>
#include <Utils.h>
#include <fbxsdk.h>
//#include <CLI11.hpp>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/scenes

constexpr size_t MaxUV = 1;
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

class CFBXTool : public CContentForgeTool
{
protected:

	struct CContext
	{
		CThreadSafeLog& Log;
	};

	struct CVertex
	{
		int        ControlPointIndex;
		FbxDouble3 Position;
		FbxDouble3 Normal;
		FbxDouble3 Tangent;
		FbxDouble3 Bitangent;
		FbxColor   Color;
		FbxVector2 UV[MaxUV];
		int        BlendIndices[MaxBonesPerVertex];
		double     BlendWeights[MaxBonesPerVertex];
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
		// FBX SDK as of 2020.0 is not guaranteed to be multithreaded
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

		const std::string MeshOutput = GetParam<std::string>(Task.Params, "MeshOutput", std::string{});
		const std::string TaskID(Task.TaskID.CStr());
		auto DestPath = fs::path(MeshOutput) / (TaskID + ".msh");
		if (!_RootDir.empty() && DestPath.is_relative())
			DestPath = fs::path(_RootDir) / DestPath;

		const double animSamplingRate = 30.0;

		// Import FBX scene from the source file

		FbxImporter* pImporter = FbxImporter::Create(pFBXManager, "");

		const auto SrcPath = Task.SrcFilePath.string();

		// Use the first argument as the filename for the importer.
		if (!pImporter->Initialize(SrcPath.c_str(), -1, pFBXManager->GetIOSettings()))
		{
			Task.Log.LogError("Failed to create FbxImporter for " + SrcPath + ": " + pImporter->GetStatus().GetErrorString());
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

		// Preprocess scene transforms and geometry to a more suitable format
		// TODO: save results back to FBX?

		ConvertTransformsToSRTRecursive(pScene->GetRootNode());
		pScene->GetRootNode()->ConvertPivotAnimationRecursive(nullptr, FbxNode::eDestinationPivot, animSamplingRate);

		{
			FbxGeometryConverter GeometryConverter(pFBXManager);

			if (!GeometryConverter.Triangulate(pScene, true))
				Task.Log.LogWarning("Couldn't triangulate some geometry");

			GeometryConverter.RemoveBadPolygonsFromMeshes(pScene);

			if (!GeometryConverter.SplitMeshesPerMaterial(pScene, true))
				Task.Log.LogWarning("Couldn't split some meshes per material");
		}

		// Export node hierarchy to DEM format

		//!!!TODO: need flags, what to export! command-line override must be provided along with .meta params
		CContext Ctx{ Task.Log };

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

	bool ExportModel(FbxMesh* pMesh, CContext& Ctx)
	{
		Ctx.Log.LogDebug("Model");

		const FbxVector4* pControlPoints = pMesh->GetControlPoints();
		const int PolyCount = pMesh->GetPolygonCount();

		const auto NormalCount = std::min(1, pMesh->GetElementNormalCount());
		if (pMesh->GetElementNormalCount() > NormalCount)
			Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " uses " + std::to_string(NormalCount) + '/' + std::to_string(pMesh->GetElementNormalCount()) + " normals");

		const auto TangentCount = std::min(1, pMesh->GetElementTangentCount());
		if (pMesh->GetElementTangentCount() > TangentCount)
			Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " uses " + std::to_string(TangentCount) + '/' + std::to_string(pMesh->GetElementTangentCount()) + " tangents");

		const auto BitangentCount = std::min(1, pMesh->GetElementBinormalCount());
		if (pMesh->GetElementBinormalCount() > BitangentCount)
			Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " uses " + std::to_string(BitangentCount) + '/' + std::to_string(pMesh->GetElementBinormalCount()) + " bitangents");

		const auto ColorCount = std::min(1, pMesh->GetElementVertexColorCount());
		if (pMesh->GetElementVertexColorCount() > ColorCount)
			Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " uses " + std::to_string(ColorCount) + '/' + std::to_string(pMesh->GetElementVertexColorCount()) + " colors");

		const auto UVCount = std::min(static_cast<int>(MaxUV), pMesh->GetElementUVCount());
		if (pMesh->GetElementUVCount() > UVCount)
			Ctx.Log.LogWarning(std::string("Mesh ") + pMesh->GetName() + " uses " + std::to_string(UVCount) + '/' + std::to_string(pMesh->GetElementUVCount()) + " UVs");

		std::vector<CVertex> Vertices;
		std::vector<unsigned int> Indices;
		Vertices.reserve(static_cast<size_t>(PolyCount * 3));
		Indices.reserve(static_cast<size_t>(PolyCount * 3));

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
				const auto VertexIndex = static_cast<unsigned int>(Vertices.size());
				Indices.push_back(VertexIndex);

				CVertex Vertex{0};
				Vertex.ControlPointIndex = pMesh->GetPolygonVertex(p, v);
				Vertex.Position = pControlPoints[Vertex.ControlPointIndex];

				if (NormalCount)
					GetVertexElement(Vertex.Normal, pMesh->GetElementNormal(), Vertex.ControlPointIndex, VertexIndex);

				if (TangentCount)
					GetVertexElement(Vertex.Tangent, pMesh->GetElementTangent(), Vertex.ControlPointIndex, VertexIndex);

				if (BitangentCount)
					GetVertexElement(Vertex.Bitangent, pMesh->GetElementBinormal(), Vertex.ControlPointIndex, VertexIndex);

				if (ColorCount)
					GetVertexElement(Vertex.Color, pMesh->GetElementVertexColor(), Vertex.ControlPointIndex, VertexIndex);

				for (int e = 0; e < MaxUV; ++e)
					GetVertexElement(Vertex.UV[e], pMesh->GetElementUV(e), Vertex.ControlPointIndex, VertexIndex);

				Vertices.push_back(std::move(Vertex));
			}
		}

		// merge vertices by control point + other params
		// patch faces with new indices
		// meshopt_generateVertexRemap
		// meshopt_remapVertexBuffer
		// meshopt_remapIndexBuffer

		// meshopt_generateShadowIndexBuffer for Z prepass

		//!!!skinning is per-control-point, so it is much better to optimize at first
		// and then fill blend params!

		// mesh
		// - build vertex declaration (check layer elements and skin deformer)
		// - collect vertices (normal, tangent (if needed), UV etc)
		// - for skinned mesh add blend indices/bones and weights
		// - collect indices (faces)
		// - use 16-bit indices when possible, warn if 32 required

		// skin
		// - collect bind pose matrices
		//???separate vertex stream for skin data? or never required.

		// Optimize geometry
		// Save geomerty

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
