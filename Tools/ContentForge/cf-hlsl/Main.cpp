#include <ContentForgeTool.h>
#include <ShaderCompiler.h>
#include <Utils.h>
#include <CLI11.hpp>
#include <iostream>
#include <thread>
#include <filesystem>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/shaders/hlsl_usm/CEGUI.hlsl.meta
// -s src/shaders/hlsl_usm/CEGUI.hlsl
// -s src/shaders

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
		CLIApp.add_option("-db", _DBPath, "Shader DB file path");
		CLIApp.add_option("-is,--inputsig", _InputSignaturesDir, "Folder where input signature binaries will be saved");
	}

	virtual int Init() override
	{
		if (_DBPath.empty())
			_DBPath = (fs::path(_RootDir) / fs::path("src/shaders/shaders.db3")).string();

		if (_InputSignaturesDir.empty())
			_InputSignaturesDir = (fs::path(_RootDir) / fs::path("shaders/sig")).string();

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

		const auto DestPath = Task.SrcFilePath.parent_path() / Task.TaskID.CStr() / ".bin";
		
		const auto Code = DEMShaderCompiler::CompileShader(
			Task.SrcFilePath.string().c_str(), DestPath.string().c_str(), _InputSignaturesDir.c_str(),
			ShaderType, Target, EntryPoint.c_str(), Defines.c_str(), Debug, Task.SrcFileData->data(), Task.SrcFileData->size());

		return Code == DEM_SHADER_COMPILER_SUCCESS;
	}
};

int main(int argc, const char** argv)
{
	CHLSLTool Tool("cf-hlsl", "HLSL to DeusExMachina resource converter", { 0, 1, 0 });
	return Tool.Execute(argc, argv);
}

/*
"Command line args:\n"
"------------------\n"
"-db [filepath]              path to persistent shader DB,\n"
"                            default: 'Shaders:ShaderDB.db3'\n"
"-d                          build shaders with debug info\n"
"-sm3                        build only old sm3.0 techs, else only\n"
"                            Unified Shader Model techs will be built\n"
"-r                          rebuild shaders even if not changed\n"

int ExitApp(int Code, bool WaitKey);
int CompileEffect(const char* pInFilePath, const char* pOutFilePath, bool Debug, EShaderModel ShaderModel);
int CompileRenderPath(const char* pInFilePath, const char* pOutFilePath, EShaderModel ShaderModel);

int main(int argc, const char** argv)
{
	std::string DB(Args.GetStringArg("-db"));
	bool Debug = Args.GetBoolArg("-d");
	bool Rebuild = Args.GetBoolArg("-r"); //!!!or if tool / DLL version changed (store in DB)!
	EShaderModel ShaderModel = Args.GetBoolArg("-sm3") ? ShaderModel_30 : ShaderModel_USM;

	if (DB.IsEmpty()) DB = "Shaders:ShaderDB.db3";
	if (Rebuild) IOSrv->DeleteFile(DB); //???!!!pass the Rebuild flag inside instead!

#ifdef _DEBUG
	std::string DLLPath = IOSrv->ResolveAssigns("Home:../DEMShaderCompiler/DEMShaderCompiler_d.dll");
#else
	std::string DLLPath = IOSrv->ResolveAssigns("Home:../DEMShaderCompiler/DEMShaderCompiler.dll");
#endif
	std::string OutputDir = PathUtils::GetAbsolutePath(IOSrv->ResolveAssigns("Home:"), IOSrv->ResolveAssigns("Shaders:Bin/"));
	if (!InitDEMShaderCompilerDLL(DLLPath.CStr(), DB.CStr(), OutputDir.CStr())) return ExitApp(ERR_MAIN_FAILED, WaitKey);

	for (size_t i = 0; i < InList.GetCount(); ++i)
	{
		const std::string& InRec = InList[i];
		const std::string& OutRec = OutList[i];

		n_msg(VL_INFO, "Compiling pair %d: '%s' -> '%s'\n", i, InRec.CStr(), OutRec.CStr());

		bool Dir = IOSrv->DirectoryExists(InRec);
		if (!Dir && IOSrv->DirectoryExists(OutRec)) return ExitApp(ERR_IN_OUT_TYPES_DONT_MATCH, WaitKey);

		if (Dir)
		{
			n_msg(VL_ERROR, "Whole directory compilation is not supported yet\n");
			return ExitApp(ERR_NOT_IMPLEMENTED_YET, WaitKey);
		}
		else
		{
			if (!IOSrv->FileExists(InRec)) return ExitApp(ERR_IN_NOT_FOUND, WaitKey);
			int Code = RenderPathes ? CompileRenderPath(InRec, OutRec, ShaderModel) : CompileEffect(InRec, OutRec, Debug, ShaderModel);
			if (Code != 0) return ExitApp(Code, WaitKey);
		}
	}

	return ExitApp(SUCCESS, WaitKey);
}
//---------------------------------------------------------------------

int ExitApp(int Code, bool WaitKey)
{
	if (!TermDEMShaderCompilerDLL())
	{
		n_msg(VL_ERROR, "DEM shader compiler was not unloaded cleanly\n");
		Code = ERR_MAIN_FAILED;
	}

	if (Code != SUCCESS) n_msg(VL_ERROR, TOOL_NAME" v"VERSION": Error occurred with code %d\n", Code);

	if (WaitKey)
	{
		Sys::Log("\nPress any key to exit...\n");
		_getch();
	}

	return Code;
}
//---------------------------------------------------------------------
*/