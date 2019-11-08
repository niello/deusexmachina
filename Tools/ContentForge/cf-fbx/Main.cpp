#include <ContentForgeTool.h>
#include <Utils.h>
#include <fbxsdk.h>
//#include <CLI11.hpp>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/scenes

class CFBXTool : public CContentForgeTool
{
protected:

	FbxManager* pFbxManager = nullptr;

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
		pFbxManager = FbxManager::Create();
		if (!pFbxManager) return 1;

		FbxIOSettings* pIOSettings = FbxIOSettings::Create(pFbxManager, IOSROOT);
		pFbxManager->SetIOSettings(pIOSettings);

		return 0;
	}

	virtual int Term() override
	{
		pFbxManager->Destroy();
		pFbxManager = nullptr;
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

		// Import FBX scene from the source file

		FbxImporter* pImporter = FbxImporter::Create(pFbxManager, "");

		const auto SrcPath = Task.SrcFilePath.string();

		// Use the first argument as the filename for the importer.
		if (!pImporter->Initialize(SrcPath.c_str(), -1, pFbxManager->GetIOSettings()))
		{
			Task.Log.LogError("Failed to create FbxImporter for " + SrcPath);
			return false;
		}

		if (pImporter->IsFBX())
		{
			int Major, Minor, Revision;
			pImporter->GetFileVersion(Major, Minor, Revision);
			Task.Log.LogDebug("Source format: FBX v" + std::to_string(Major) + '.' + std::to_string(Minor) + '.' + std::to_string(Revision));
		}

		FbxScene* pScene = FbxScene::Create(pFbxManager, "SourceScene");
		const bool Imported = pImporter->Import(pScene);
		pImporter->Destroy();			

		if (!Imported)
		{
			Task.Log.LogError("Failed to import " + SrcPath);
			return false;
		}

		//!!!DBG TMP!
		if (FbxNode* pRootNode = pScene->GetRootNode())
		{
			for (int i = 0; i < pRootNode->GetChildCount(); ++i)
				PrintNode(pRootNode->GetChild(i), 0);
		}

		return true;
	}


	//======================

	std::string GetAttributeTypeName(FbxNodeAttribute::EType type)
	{
		switch(type)
		{
			case FbxNodeAttribute::eUnknown: return "unidentified";
			case FbxNodeAttribute::eNull: return "null";
			case FbxNodeAttribute::eMarker: return "marker";
			case FbxNodeAttribute::eSkeleton: return "skeleton";
			case FbxNodeAttribute::eMesh: return "mesh";
			case FbxNodeAttribute::eNurbs: return "nurbs";
			case FbxNodeAttribute::ePatch: return "patch";
			case FbxNodeAttribute::eCamera: return "camera";
			case FbxNodeAttribute::eCameraStereo: return "stereo";
			case FbxNodeAttribute::eCameraSwitcher: return "camera switcher";
			case FbxNodeAttribute::eLight: return "light";
			case FbxNodeAttribute::eOpticalReference: return "optical reference";
			case FbxNodeAttribute::eOpticalMarker: return "marker";
			case FbxNodeAttribute::eNurbsCurve: return "nurbs curve";
			case FbxNodeAttribute::eTrimNurbsSurface: return "trim nurbs surface";
			case FbxNodeAttribute::eBoundary: return "boundary";
			case FbxNodeAttribute::eNurbsSurface: return "nurbs surface";
			case FbxNodeAttribute::eShape: return "shape";
			case FbxNodeAttribute::eLODGroup: return "lodgroup";
			case FbxNodeAttribute::eSubDiv: return "subdiv";
			default: return "unknown";
		}
	}

	void PrintAttribute(FbxNodeAttribute* pAttribute, int depth)
	{
		if (!pAttribute) return;

		auto typeName = GetAttributeTypeName(pAttribute->GetAttributeType());

		for(int i = 0; i < depth; i++)
			printf(" ");

		printf("<attribute type='%s' name='%s'/>\n", typeName.c_str(), pAttribute->GetName());
	}

	void PrintNode(FbxNode* pNode, int depth)
	{
		for(int i = 0; i < depth; i++)
			printf(" ");

		const char* nodeName = pNode->GetName();
		FbxDouble3 translation = pNode->LclTranslation.Get();
		FbxDouble3 rotation = pNode->LclRotation.Get();
		FbxDouble3 scaling = pNode->LclScaling.Get();

		// Print the contents of the node.
		printf("<node name='%s' translation='(%f, %f, %f)' rotation='(%f, %f, %f)' scaling='(%f, %f, %f)'>\n",
			nodeName,
			translation[0], translation[1], translation[2],
			rotation[0], rotation[1], rotation[2],
			scaling[0], scaling[1], scaling[2]
		);

		// Print the node's attributes.
		for(int i = 0; i < pNode->GetNodeAttributeCount(); i++)
			PrintAttribute(pNode->GetNodeAttributeByIndex(i), depth + 1);

		// Recursively print the children.
		for(int j = 0; j < pNode->GetChildCount(); j++)
			PrintNode(pNode->GetChild(j), depth + 1);

		for(int i = 0; i < depth; i++)
			printf(" ");

		printf("</node>\n");
	}

	//=======================


};

int main(int argc, const char** argv)
{
	CFBXTool Tool("cf-fbx", "FBX to DeusExMachina resource converter", { 0, 1, 0 });
	return Tool.Execute(argc, argv);
}
