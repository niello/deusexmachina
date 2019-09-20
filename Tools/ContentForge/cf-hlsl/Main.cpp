#include <IO/IOServer.h>
#include <IO/PathUtils.h>
#include <Data/StringTokenizer.h>
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <Data/HRDParser.h>
#include <Data/Buffer.h>
#include <DEMShaderCompiler/DEMShaderCompilerDLL.h>
#include <ConsoleApp.h>

// Debug args:
// -waitkey -v 5 -proj "..\..\..\..\InsanePoet\Content" -in "SrcShaders:Effects\PBR.hrd" -out "Shaders:USM\Effects\PBR.eff"
// -waitkey -v 5 -proj "..\..\..\..\InsanePoet\Content" -in "SrcShaders:Effects\PBRAlphaTest.hrd" -out "Shaders:USM\Effects\PBRAlphaTest.eff"
// -waitkey -v 5 -proj "..\..\..\..\InsanePoet\Content" -in "SrcShaders:Effects\PBRSplatting.hrd" -out "Shaders:USM\Effects\PBRSplatting.eff"
// -waitkey -v 5 -proj "..\..\..\..\InsanePoet\Content" -in "SrcShaders:Effects\DepthOpaque.hrd" -out "Shaders:USM\Effects\DepthOpaque.eff"
// -waitkey -v 5 -sm3 -proj "..\..\..\..\InsanePoet\Content" -in "SrcShaders:Effects\PBR.hrd" -out "Shaders:SM_3_0\Effects\PBR.eff"
// -waitkey -v 5 -rp -proj "..\..\..\..\InsanePoet\Content" -eff "Shaders:USM/Effects" -in "SrcShaders:D3D11Forward.hrd" -out "Shaders:USM\D3D11Forward.rp"
// -waitkey -v 5 -rp -sm3 -proj "..\..\..\..\InsanePoet\Content" -eff "Shaders:SM_3_0/Effects" -in "SrcShaders:D3D11Forward.hrd" -out "Shaders:SM_3_0\D3D11Forward.rp"
// -v 0 -proj C:/Niello/Projects/GameDev/Dev/InsanePoet/Content -in C:/Niello/Projects/GameDev/Dev/InsanePoet/Content/Src/Shaders/Effects/PBR.hrd -out C:/Niello/Projects/GameDev/Dev/InsanePoet/Content/Export/Shaders/Effects/PBR.eff
// -waitkey -v 5 -proj C:/Niello/Projects/GameDev/Dev/InsanePoet/Content -in C:/Niello/Projects/GameDev/Dev/InsanePoet/Content/Src/Shaders/Effects/PBR.hrd -out C:/Niello/Projects/GameDev/Dev/InsanePoet/Content/Export/Shaders/Effects/PBR.eff
// WD = $(ProjectDir) -proj "..\..\..\..\..\InsanePoet\Content"
// WD = $(TargetDir) -proj "..\..\..\..\InsanePoet\Content"

#define TOOL_NAME	"CFShader"
#define VERSION		"1.0"

int									Verbose = VL_ERROR;
CString								RootPath;
CHashTable<CString, Data::CFourCC>	ClassToFOURCC;

int ExitApp(int Code, bool WaitKey);
int CompileEffect(const char* pInFilePath, const char* pOutFilePath, bool Debug, EShaderModel ShaderModel);
int CompileRenderPath(const char* pInFilePath, const char* pOutFilePath, EShaderModel ShaderModel);

int main(int argc, const char** argv)
{
	nCmdLineArgs Args(argc, argv);

	bool WaitKey = Args.GetBoolArg("-waitkey");
	Verbose = Args.GetIntArg("-v");
	bool RenderPathes = Args.GetBoolArg("-rp");

	bool Help = Args.GetBoolArg("-help") || Args.GetBoolArg("/?");

	if (Help)
	{
		printf(	TOOL_NAME" v"VERSION" - DeusExMachina shader effect compiler tool\n"
				"Command line args:\n"
				"------------------\n"
				"-help OR /?                 show this help\n"
				"-proj [path]                project path\n"
				"-root [path]                root path to shaders, typically the same as\n"
				"                            'Shaders:' assign\n"
				"-eff [path]                 'Effects:' assign for render path compilation\n"
				"-in [filepath{;filepath}]   input file(s)\n"
				"-out [filepath{;filepath}]  output file(s), count must be the same\n"
				"-db [filepath]              path to persistent shader DB,\n"
				"                            default: 'Shaders:ShaderDB.db3'\n"
				"-d                          build shaders with debug info\n"
				"-sm3                        build only old sm3.0 techs, else only\n"
				"                            Unified Shader Model techs will be built\n"
				"-rp                         render path compilation requested\n"
				"-r                          rebuild shaders even if not changed\n"
				"-waitkey                    wait for a key press when tool has finished\n"
				"-v [verbosity]              output verbosity level, [ 0 .. 5 ]\n");

		return ExitApp(SUCCESS_HELP, WaitKey);
	}

	const char* pIn = Args.GetStringArg("-in");
	const char* pOut = Args.GetStringArg("-out");
	RootPath = Args.GetStringArg("-root");
	const char* pProjPath = Args.GetStringArg("-proj");
	CString DB(Args.GetStringArg("-db"));
	bool Debug = Args.GetBoolArg("-d");
	bool SM30 = Args.GetBoolArg("-sm3");
	bool Rebuild = Args.GetBoolArg("-r"); //!!!or if tool / DLL version changed (store in DB)!

	EShaderModel ShaderModel = SM30 ? ShaderModel_30 : ShaderModel_USM;

	if (!pIn || !pOut || !*pIn || !*pOut) return ExitApp(ERR_INVALID_CMD_LINE, WaitKey);

	IO::CIOServer IOServer;

	if (pProjPath) IOSrv->SetAssign("Proj", pProjPath);
	else IOSrv->SetAssign("Proj", IOSrv->GetAssign("Home"));

	if (RenderPathes)
	{
		const char* pEffPath = Args.GetStringArg("-eff");
		if (pEffPath) IOSrv->SetAssign("Effects", pEffPath);
		else IOSrv->SetAssign("Effects", IOSrv->GetAssign("Shaders"));
	
		Data::CBuffer Buffer;
		if (!IOSrv->LoadFileToBuffer("Proj:ClassToFOURCC.hrd", Buffer)) return ERR_INVALID_DATA;

		Data::PParams ClassToFOURCCDesc;
		Data::CHRDParser Parser;
		if (!Parser.ParseBuffer((const char*)Buffer.GetPtr(), Buffer.GetSize(), ClassToFOURCCDesc)) return ERR_INVALID_DATA;

		Data::PDataArray ClassToFOURCCArray;
		if (ClassToFOURCCDesc.IsValidPtr() &&
			ClassToFOURCCDesc->Get<Data::PDataArray>(ClassToFOURCCArray, CStrID("List")) &&
			ClassToFOURCCArray->GetCount())
		{
			for (UPTR i = 0; i < ClassToFOURCCArray->GetCount(); ++i)
			{
				Data::PParams Prm = ClassToFOURCCArray->Get<Data::PParams>(i);
				ClassToFOURCC.Add(Prm->Get<CString>(CStrID("Name")), Data::CFourCC(Prm->Get<CString>(CStrID("Code")).CStr()));
			}
		}
	}

	{
		Data::CBuffer Buffer;
		Data::CHRDParser Parser;
		if (IOSrv->LoadFileToBuffer("Proj:PathList.hrd", Buffer))
		{
			Data::PParams PathList;
			if (!Parser.ParseBuffer((const char*)Buffer.GetPtr(), Buffer.GetSize(), PathList)) return ERR_IO_READ;

			if (PathList.IsValidPtr())
				for (UPTR i = 0; i < PathList->GetCount(); ++i)
					IOSrv->SetAssign(PathList->Get(i).GetName().CStr(), IOSrv->ResolveAssigns(PathList->Get<CString>(i)));
		}

		if (IOSrv->LoadFileToBuffer("Proj:SrcPathList.hrd", Buffer))
		{
			Data::PParams PathList;
			if (!Parser.ParseBuffer((const char*)Buffer.GetPtr(), Buffer.GetSize(), PathList)) return ERR_IO_READ;

			if (PathList.IsValidPtr())
				for (UPTR i = 0; i < PathList->GetCount(); ++i)
					IOSrv->SetAssign(PathList->Get(i).GetName().CStr(), IOSrv->ResolveAssigns(PathList->Get<CString>(i)));
		}
	}

	if (RootPath.IsEmpty()) RootPath = IOSrv->ResolveAssigns("Proj:Src/Shaders");
	else RootPath = IOSrv->ResolveAssigns(RootPath);
	RootPath.Trim();
	RootPath.Replace('\\', '/');
	if (RootPath[RootPath.GetLength() - 1] != '/') RootPath += '/';
	if (RootPath.IsEmpty()) return ExitApp(ERR_INVALID_CMD_LINE, WaitKey);

	if (DB.IsEmpty()) DB = "Shaders:ShaderDB.db3";
	DB = IOSrv->ResolveAssigns(DB);

	if (Rebuild) IOSrv->DeleteFile(DB);

#ifdef _DEBUG
	CString DLLPath = IOSrv->ResolveAssigns("Home:../DEMShaderCompiler/DEMShaderCompiler_d.dll");
#else
	CString DLLPath = IOSrv->ResolveAssigns("Home:../DEMShaderCompiler/DEMShaderCompiler.dll");
#endif
	CString OutputDir = PathUtils::GetAbsolutePath(IOSrv->ResolveAssigns("Home:"), IOSrv->ResolveAssigns("Shaders:Bin/"));
	if (!InitDEMShaderCompilerDLL(DLLPath.CStr(), DB.CStr(), OutputDir.CStr())) return ExitApp(ERR_MAIN_FAILED, WaitKey);

	CArray<CString> InList, OutList;

	{
		Data::CStringTokenizer StrTok(pIn);
		while (StrTok.GetNextToken(';'))
			InList.Add(CString(StrTok.GetCurrToken()));
	}

	{
		Data::CStringTokenizer StrTok(pOut);
		while (StrTok.GetNextToken(';'))
			OutList.Add(CString(StrTok.GetCurrToken()));
	}

	if (InList.GetCount() != OutList.GetCount()) return ExitApp(ERR_INVALID_CMD_LINE, WaitKey);

	for (UPTR i = 0; i < InList.GetCount(); ++i)
	{
		const CString& InRec = InList[i];
		const CString& OutRec = OutList[i];

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
