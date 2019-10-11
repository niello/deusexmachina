#include <ContentForgeTool.h>
#include <ShaderCompiler.h>
#include <Utils.h>
#include <CLI11.hpp>
#include <thread>
#include <mutex>
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

	EVerbosity _Verbosity;
	std::string _Prefix;
	std::ostringstream _Stream; // Cache logs in a separate stream for thread safety
	char _LineEnd = '\n';

public:

	CLog(const std::string& TaskID, EVerbosity Verbosity)
		: _Verbosity(Verbosity)
	{
		_Prefix = "[DLL " + TaskID + "] ";
		_LineEnd = _Stream.widen('\n');
	}

	std::ostringstream& GetStream() { return _Stream; }

	virtual void Log(EVerbosity Level, const char* pMessage) override
	{
		if (_Verbosity >= Level)
			_Stream << GetSeverityPrefix(Level) << _Prefix << pMessage << _LineEnd;
	}
};

class CHLSLTool : public CContentForgeTool
{
private:

	std::mutex COutMutex;

public:

	std::string _DBPath;
	std::string _InputSignaturesDir;
	bool _ForceRecompilation = false;

	CHLSLTool(const std::string& Name, const std::string& Desc, CVersion Version) :
		CContentForgeTool(Name, Desc, Version)
	{
		// Set default before parsing command line
		_RootDir = "../../../content";
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

		CLog Log(TaskID, static_cast<EVerbosity>(_LogVerbosity));

		const auto LineEnd = Log.GetStream().widen('\n');

		// FIXME: can move to the common code
		if (_LogVerbosity >= EVerbosity::Debug)
		{
			Log.GetStream() << "Source: " << Task.SrcFilePath.generic_string() << LineEnd;
			Log.GetStream() << "Task: " << Task.TaskID.CStr() << LineEnd;
			Log.GetStream() << "Thread: " << std::this_thread::get_id() << LineEnd;
		}

		const auto Code = DEMShaderCompiler::CompileShader(_RootDir.c_str(),
			SrcPath.string().c_str(), DestPath.string().c_str(), _InputSignaturesDir.c_str(),
			ShaderType, Target, EntryPoint.c_str(), Defines.c_str(), Debug, _ForceRecompilation, Task.SrcFileData->data(), Task.SrcFileData->size(), &Log);

		const auto LoggedString = Log.GetStream().str();
		if (!LoggedString.empty())
		{
			// Flush cached logs to stdout
			// TODO: move log flushing to the end of CContentForgeTool::Execute? No locking needed at all there.
			std::lock_guard<std::mutex> Lock(COutMutex);
			std::cout << LoggedString;
		}

		// FIXME: must be thread-safe, also can move to the common code
		if (_LogVerbosity >= EVerbosity::Debug)
			std::cout << "Status: " << ((Code == DEM_SHADER_COMPILER_SUCCESS) ? "OK" : "FAIL") << LineEnd << LineEnd;

		return Code == DEM_SHADER_COMPILER_SUCCESS;
	}
};

int main(int argc, const char** argv)
{
	CHLSLTool Tool("cf-hlsl", "HLSL to DeusExMachina resource converter", { 0, 1, 0 });
	return Tool.Execute(argc, argv);
}
