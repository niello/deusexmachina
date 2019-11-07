#include <ContentForgeTool.h>
#include <Utils.h>
#include <fbxsdk.h>
//#include <CLI11.hpp>
//#include <thread>
//#include <filesystem>

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
		// FIXME: must handle duplicate targets (for example 2 metafiles writing the same resulting file, like depth_atest_ps)
		// FIXME: CStrID
		return false;
	}

	virtual int Init() override
	{
		pFbxManager = FbxManager::Create();
		return pFbxManager ? 0 : 1;
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

		const std::string Output = GetParam<std::string>(Task.Params, "Output", std::string{});
		const std::string TaskID(Task.TaskID.CStr());
		auto DestPath = fs::path(Output) / (TaskID + ".mtl");
		if (!_RootDir.empty() && DestPath.is_relative())
			DestPath = fs::path(_RootDir) / DestPath;

		return true;
	}
};

int main(int argc, const char** argv)
{
	CFBXTool Tool("cf-fbx", "FBX to DeusExMachina resource converter", { 0, 1, 0 });
	return Tool.Execute(argc, argv);
}
