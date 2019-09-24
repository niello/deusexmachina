#include <ContentForgeTool.h>
#include <ShaderCompiler.h>
#include <Utils.h>
#include <CLI11.hpp>
#include <thread>
#include <filesystem>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/shaders/hlsl_usm/CEGUI.hlsl.meta
// -s src/shaders/hlsl_usm/CEGUI.hlsl
// -s src/shaders

class CLog : public DEMShaderCompiler::ILogDelegate
{
private:

	std::string Prefix;

public:

	CLog(const std::string& TaskID)
	{
		Prefix = "[DLL " + TaskID + "] ";
	}

	virtual void Log(const char* pMessage) override
	{
		// FIXME: not thread safe, just for testing
		std::cout << Prefix << pMessage << std::endl;
	}
};

class CHLSLTool : public CContentForgeTool
{
public:

	std::string _DBPath;
	std::string _InputSignaturesDir;

	CHLSLTool(const std::string& Name, const std::string& Desc, CVersion Version) :
		CContentForgeTool(Name, Desc, Version)
	{
		// Set default before parsing command line
		_RootDir = "../../../content";
	}

	virtual void ProcessCommandLine(CLI::App& CLIApp) override
	{
		CContentForgeTool::ProcessCommandLine(CLIApp);
		CLIApp.add_option("--db", _DBPath, "Shader DB file path");
		CLIApp.add_option("--is,--inputsig", _InputSignaturesDir, "Folder where input signature binaries will be saved");
	}

	virtual int Init() override
	{
		if (_DBPath.empty())
			_DBPath = (fs::path(_RootDir) / fs::path("src/shaders/shaders.db3")).string();

		if (_InputSignaturesDir.empty())
			_InputSignaturesDir = "shaders/sig";

		if (!DEMShaderCompiler::Init(_DBPath.c_str())) return 1;

		return 0;
	}

	virtual bool ProcessTask(CContentForgeTask& Task) override
	{
		if (_LogVerbosity >= EVerbosity::Debug)
		{
			std::cout << "Source: " << Task.SrcFilePath.generic_string() << std::endl;
			std::cout << "Task: " << Task.TaskID.CStr() << std::endl;
			std::cout << "Thread: " << std::this_thread::get_id() << std::endl;
		}

		const std::string EntryPoint = GetParam<std::string>(Task.Params, "Entry", std::string{});
		if (EntryPoint.empty()) return false;

		const std::string Output = GetParam<std::string>(Task.Params, "Output", std::string{});
		const int Target = GetParam<int>(Task.Params, "Target", 0);
		const std::string Defines = GetParam<std::string>(Task.Params, "Defines", std::string{});
		const bool Debug = GetParam<bool>(Task.Params, "Debug", false);

		EShaderType ShaderType;
		const CStrID Type = GetParam<CStrID>(Task.Params, "Type", CStrID::Empty);
		if (Type == "Vertex") ShaderType = ShaderType_Vertex;
		else if (Type == "Pixel") ShaderType = ShaderType_Pixel;
		else if (Type == "Geometry") ShaderType = ShaderType_Geometry;
		else if (Type == "Hull") ShaderType = ShaderType_Hull;
		else if (Type == "Domain") ShaderType = ShaderType_Domain;
		else return false;

		const auto SrcPath = fs::relative(Task.SrcFilePath, _RootDir);

		const std::string TaskID(Task.TaskID.CStr());
		const auto DestPath = fs::path(Output) / (TaskID + ".bin");

		CLog Log(TaskID);

		const auto Code = DEMShaderCompiler::CompileShader(_RootDir.c_str(),
			SrcPath.string().c_str(), DestPath.string().c_str(), _InputSignaturesDir.c_str(),
			ShaderType, Target, EntryPoint.c_str(), Defines.c_str(), Debug, Task.SrcFileData->data(), Task.SrcFileData->size(), &Log);

		return Code == DEM_SHADER_COMPILER_SUCCESS;
	}
};

int main(int argc, const char** argv)
{
	CHLSLTool Tool("cf-hlsl", "HLSL to DeusExMachina resource converter", { 0, 1, 0 });
	return Tool.Execute(argc, argv);
}
