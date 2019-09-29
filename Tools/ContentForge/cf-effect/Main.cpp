#include <ContentForgeTool.h>
#include <Utils.h>
#include <HRDParser.h>
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
		// TODO: check whether the metafile can be processed by this tool

		const std::string Output = GetParam<std::string>(Task.Params, "Output", std::string{});
		const std::string TaskID(Task.TaskID.CStr());
		const auto DestPath = fs::path(Output) / (TaskID + ".eff");

		// FIXME: need MT-safe logger instead of cout!
		const auto LineEnd = std::cout.widen('\n');
		if (_LogVerbosity >= EVerbosity::Debug)
		{
			std::cout << "Source: " << Task.SrcFilePath.generic_string() << LineEnd;
			std::cout << "Task: " << Task.TaskID.CStr() << LineEnd;
			std::cout << "Thread: " << std::this_thread::get_id() << LineEnd;
		}

		// Read effect hrd

		Data::CParams Desc;
		{
			std::vector<char> In;
			if (!ReadAllFile(Task.SrcFilePath.string().c_str(), In, false))
			{
				if (_LogVerbosity >= EVerbosity::Errors)
					std::cout << Task.SrcFilePath.generic_string() << " reading error" << LineEnd;
				return false;
			}

			Data::CHRDParser Parser;
			if (!Parser.ParseBuffer(In.data(), In.size(), Desc))
			{
				if (_LogVerbosity >= EVerbosity::Errors)
					std::cout << Task.SrcFilePath.generic_string() << " HRD parsing error" << LineEnd;
				return false;
			}
		}

		// Get material type
		// Get global params
		// Get material params

		// For each tech:
		// discard empty techs
		// get shader format
		// verify that all shaders are of compatible format, or discard a tech
		// get feature level (max through all used shaders)
		// get max light count (-1 = any, 0 = no lights in this pass)
		// inside each format sort by the feature level desc

		//!!!light count is an inbuilt define! System builds shader variations!
		//???need or instead of switching shader and flushing pipeline just render max light count,
		//skipping "-1" light index as "no light"?

		//???prebuild material constant table? or will be built in runtime?

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
