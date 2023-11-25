#include <ContentForgeTool.h>
#include <ShaderCompiler.h>
#include <Utils.h>
#include <ParamsUtils.h>
#include <CLI11.hpp>
#include <thread>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/shaders/hlsl_usm/CEGUI.hlsl.meta -cfg release
// -s src/shaders/hlsl_usm/CEGUI.hlsl -cfg release
// -s src/shaders --cfg release
// -s src/shaders --cfg debug -d

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
	std::string _ConfigName;
	bool _ForceRecompilation = false;
	bool _Debug = false;

	using CContentForgeTool::CContentForgeTool;

	virtual int Init() override
	{
		if (_OutDir.empty())
			_OutDir = (fs::path(_RootDir) / "shaders" / _ConfigName).string();

		if (_DBPath.empty())
			_DBPath = (fs::path(_RootDir) / "src/shaders/shaders.db3").string();

		if (_InputSignaturesDir.empty())
			_InputSignaturesDir = (fs::relative(fs::path(_OutDir) / "sig", _RootDir)).string();

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
		CLIApp.add_option("--cfg", _ConfigName, "Configuration name. Multiple configs with different flags can coexist in a cache.");
		CLIApp.add_flag("-r", _ForceRecompilation, "Force recompilation");
		CLIApp.add_flag("-d", _Debug, "Compile shaders for debugging");
	}

	virtual ETaskResult ProcessTask(CContentForgeTask& Task) override
	{
		const std::string EntryPoint = ParamsUtils::GetParam<std::string>(Task.Params, "Entry", std::string{});
		if (EntryPoint.empty()) return ETaskResult::Failure;

		const int Target = ParamsUtils::GetParam<int>(Task.Params, "Target", 0);
		const std::string Defines = ParamsUtils::GetParam<std::string>(Task.Params, "Defines", std::string{});
		const bool Debug = _Debug || ParamsUtils::GetParam<bool>(Task.Params, "Debug", false);

		EShaderType ShaderType;
		const CStrID Type = ParamsUtils::GetParam<CStrID>(Task.Params, "Type", CStrID::Empty);
		if (Type == "Vertex") ShaderType = ShaderType_Vertex;
		else if (Type == "Pixel") ShaderType = ShaderType_Pixel;
		else if (Type == "Geometry") ShaderType = ShaderType_Geometry;
		else if (Type == "Hull") ShaderType = ShaderType_Hull;
		else if (Type == "Domain") ShaderType = ShaderType_Domain;
		else return ETaskResult::Failure;

		std::vector<char> SrcFileData;
		if (!ReadAllFile(Task.SrcFilePath.string().c_str(), SrcFileData))
		{
			Task.Log.LogError("Error reading shader source " + Task.SrcFilePath.generic_string());
			return ETaskResult::Failure;
		}

		const auto SrcPath = fs::relative(Task.SrcFilePath, _RootDir);
		const auto DestPath = fs::relative(GetOutputPath(Task.Params) / (Task.TaskID.ToString() + ".bin"), _RootDir);

		CDLLLog Log(&Task.Log);

		const auto Code = DEMShaderCompiler::CompileShader(_RootDir.c_str(),
			SrcPath.string().c_str(), DestPath.string().c_str(), _InputSignaturesDir.c_str(),
			ShaderType, Target, _ConfigName.c_str(), EntryPoint.c_str(), Defines.c_str(), Debug, _ForceRecompilation, SrcFileData.data(), SrcFileData.size(), &Log);

		return(Code == DEM_SHADER_COMPILER_SUCCESS) ? ETaskResult::Success :
			(Code == DEM_SHADER_COMPILER_UP_TO_DATE) ? ETaskResult::UpToDate :
			ETaskResult::Failure;
	}
};

int main(int argc, const char** argv)
{
	CHLSLTool Tool("cf-hlsl", "HLSL to DeusExMachina resource converter", { 0, 2, 0 });
	return Tool.Execute(argc, argv);
}
