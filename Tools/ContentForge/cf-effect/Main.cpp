#include <ContentForgeTool.h>
#include <Utils.h>
//#include <CLI11.hpp>
#include <thread>
//#include <mutex>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/effects

class CEffectTool : public CContentForgeTool
{
private:

	//std::mutex COutMutex;

public:

	CEffectTool(const std::string& Name, const std::string& Desc, CVersion Version) :
		CContentForgeTool(Name, Desc, Version)
	{
		// Set default before parsing command line
		_RootDir = "../../../content";
	}

	virtual int Init() override
	{
		return 0;
	}

	virtual bool SupportsMultithreading() const override
	{
		// FIXME: must handle duplicate targets (for example 2 metafiles writing the same resulting file, like depth_atest_ps)
		return false;
	}

	//virtual void ProcessCommandLine(CLI::App& CLIApp) override
	//{
	//	CContentForgeTool::ProcessCommandLine(CLIApp);
	//	CLIApp.add_option("--db", _DBPath, "Shader DB file path");
	//	CLIApp.add_option("--is,--inputsig", _InputSignaturesDir, "Folder where input signature binaries will be saved");
	//}

	virtual bool ProcessTask(CContentForgeTask& Task) override
	{
		const std::string EntryPoint = GetParam<std::string>(Task.Params, "Entry", std::string{});
		if (EntryPoint.empty()) return false;

		const std::string Output = GetParam<std::string>(Task.Params, "Output", std::string{});
		const int Target = GetParam<int>(Task.Params, "Target", 0);
		const std::string Defines = GetParam<std::string>(Task.Params, "Defines", std::string{});
		const bool Debug = GetParam<bool>(Task.Params, "Debug", false);

		const auto SrcPath = fs::relative(Task.SrcFilePath, _RootDir);

		const std::string TaskID(Task.TaskID.CStr());
		const auto DestPath = fs::path(Output) / (TaskID + ".bin");

		const auto LineEnd = std::cout.widen('\n');

		if (_LogVerbosity >= EVerbosity::Debug)
		{
			std::cout << "Source: " << Task.SrcFilePath.generic_string() << LineEnd;
			std::cout << "Task: " << Task.TaskID.CStr() << LineEnd;
			std::cout << "Thread: " << std::this_thread::get_id() << LineEnd;
		}

		// 

		//if (_LogVerbosity >= EVerbosity::Debug)
		//	std::cout << "Status: " << (Ok ? "OK" : "FAIL") << LineEnd << LineEnd;

		return true;
	}
};

int main(int argc, const char** argv)
{
	CEffectTool Tool("cf-effect", "DeusExMachina rendering effect compiler", { 0, 1, 0 });
	return Tool.Execute(argc, argv);
}
