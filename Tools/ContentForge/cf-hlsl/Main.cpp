#include <ContentForgeTool.h>
#include <ShaderCompiler.h>
#include <Utils.h>
#include <ParamsUtils.h>
#include <CLI11.hpp>
#include <thread>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/shaders/hlsl_usm/CEGUI.hlsl.meta
// -s src/shaders/hlsl_usm/CEGUI.hlsl
// -s src/shaders

class CDLLLog : public DEMShaderCompiler::ILogDelegate
{
private:

	CThreadSafeLog* _pLog = nullptr;

public:

	CDLLLog(CThreadSafeLog* pLog) : _pLog(pLog) {}

	std::ostringstream& GetStream() { return _pLog->GetStream(); }

	virtual void Log(EVerbosity Level, const char* pMessage) override
	{
		if (_pLog && _pLog->GetVerbosity() >= Level)
			_pLog->GetStream() << GetSeverityPrefix(Level) << "[DLL] " << pMessage << _pLog->GetLineEnd();
	}
};

class CHLSLTool : public CContentForgeTool
{
private:

public:

	std::string _DBPath;
	std::string _InputSignaturesDir;
	bool _ForceRecompilation = false;

	CHLSLTool(const std::string& Name, const std::string& Desc, CVersion Version) :
		CContentForgeTool(Name, Desc, Version)
	{
	}

	virtual int Init() override
	{
		if (_DBPath.empty())
			_DBPath = (fs::path(_RootDir) / fs::path("src/shaders/shaders.db3")).string();

		if (_InputSignaturesDir.empty())
			_InputSignaturesDir = "shaders/d3d_usm/sig";

		if (!DEMShaderCompiler::Init(_DBPath.c_str())) return 1;

		return 0;
	}

	virtual bool SupportsMultithreading() const override
	{
		// FIXME: must handle duplicate targets (for example 2 metafiles writing the same resulting file, like depth_atest_ps)
		// FIXME: CStrID
		return false;

		// DLL is guaranteed to be thread-safe, but we could add a check: sqlite3_threadsafe()
		//return true;
	}

	virtual void ProcessCommandLine(CLI::App& CLIApp) override
	{
		CContentForgeTool::ProcessCommandLine(CLIApp);
		CLIApp.add_option("--db", _DBPath, "Shader DB file path");
		CLIApp.add_option("--is,--inputsig", _InputSignaturesDir, "Folder where input signature binaries will be saved");
		CLIApp.add_flag("-r", _ForceRecompilation, "Force recompilation");
	}

	virtual bool ProcessTask(CContentForgeTask& Task) override
	{
		const std::string EntryPoint = ParamsUtils::GetParam<std::string>(Task.Params, "Entry", std::string{});
		if (EntryPoint.empty()) return false;

		const std::string Output = ParamsUtils::GetParam<std::string>(Task.Params, "Output", std::string{});
		const int Target = ParamsUtils::GetParam<int>(Task.Params, "Target", 0);
		const std::string Defines = ParamsUtils::GetParam<std::string>(Task.Params, "Defines", std::string{});
		const bool Debug = ParamsUtils::GetParam<bool>(Task.Params, "Debug", false);

		EShaderType ShaderType;
		const CStrID Type = ParamsUtils::GetParam<CStrID>(Task.Params, "Type", CStrID::Empty);
		if (Type == "Vertex") ShaderType = ShaderType_Vertex;
		else if (Type == "Pixel") ShaderType = ShaderType_Pixel;
		else if (Type == "Geometry") ShaderType = ShaderType_Geometry;
		else if (Type == "Hull") ShaderType = ShaderType_Hull;
		else if (Type == "Domain") ShaderType = ShaderType_Domain;
		else return false;

		const auto SrcPath = fs::relative(Task.SrcFilePath, _RootDir);

		const std::string TaskID(Task.TaskID.CStr());
		const auto DestPath = fs::path(Output) / (TaskID + ".bin");

		CDLLLog Log(&Task.Log);

		const auto Code = DEMShaderCompiler::CompileShader(_RootDir.c_str(),
			SrcPath.string().c_str(), DestPath.string().c_str(), _InputSignaturesDir.c_str(),
			ShaderType, Target, EntryPoint.c_str(), Defines.c_str(), Debug, _ForceRecompilation, Task.SrcFileData->data(), Task.SrcFileData->size(), &Log);

		return Code == DEM_SHADER_COMPILER_SUCCESS;
	}
};

int main(int argc, const char** argv)
{
	CHLSLTool Tool("cf-hlsl", "HLSL to DeusExMachina resource converter", { 1, 0, 0 });
	return Tool.Execute(argc, argv);
}
