#include <IO/IOServer.h>
#include <Data/StringTokenizer.h>
#include <Data/Params.h>
#include <Data/HRDParser.h>
#include <Data/Buffer.h>
#include <ConsoleApp.h>
#include <ShaderDB.h>

// Debug args:
// -waitkey -v 5 -r -proj "..\..\..\..\..\InsanePoet\Content" -in "SrcShaders:SM_4_0\CEGUI.hrd" -out "Shaders:SM_4_0\CEGUI.eff"
// -waitkey -v 5 -r -proj "..\..\..\..\..\InsanePoet\Content" -in "SrcShaders:SM_3_0\CEGUI.hrd" -out "Shaders:SM_3_0\CEGUI.eff"
// -waitkey -v 5 -r -proj "..\..\..\..\..\InsanePoet\Content" -in "SrcShaders:SM_3_0\CEGUI.hrd;SrcShaders:SM_4_0\CEGUI.hrd" -out "Shaders:SM_3_0\CEGUI.eff;Shaders:SM_4_0\CEGUI.eff"

#define TOOL_NAME	"CFShader"
#define VERSION		"1.0"

int		Verbose = VL_ERROR;
CString	RootPath;

int ExitApp(int Code, bool WaitKey);
int CompileEffect(const char* pInFilePath, const char* pOutFilePath, bool Debug);

int main(int argc, const char** argv)
{
	nCmdLineArgs Args(argc, argv);

	bool WaitKey = Args.GetBoolArg("-waitkey");
	Verbose = Args.GetIntArg("-v");

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
				"-in [filepath{;filepath}]   input file(s)\n"
				"-out [filepath{;filepath}]  output file(s), count must be the same\n"
				"-db [filepath]              path to persistent shader DB,\n"
				"                            default: -root + 'ShaderDB.db3'\n"
				"-d                          build shaders with debug info\n"
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
	bool Rebuild = Args.GetBoolArg("-r"); //!!!or if tool version changed (store in DB)!

	if (!pIn || !pOut || !*pIn || !*pOut) return ExitApp(ERR_INVALID_CMD_LINE, WaitKey);

	IO::CIOServer IOServer;

	if (pProjPath) IOSrv->SetAssign("Proj", pProjPath);
	else IOSrv->SetAssign("Proj", IOSrv->GetAssign("Home"));

	{
		Data::CBuffer Buffer;
		Data::CHRDParser Parser;
		if (IOSrv->LoadFileToBuffer("Proj:PathList.hrd", Buffer))
		{
			Data::PParams PathList;
			if (!Parser.ParseBuffer((LPCSTR)Buffer.GetPtr(), Buffer.GetSize(), PathList)) return ERR_IO_READ;

			if (PathList.IsValidPtr())
				for (int i = 0; i < PathList->GetCount(); ++i)
					IOSrv->SetAssign(PathList->Get(i).GetName().CStr(), IOSrv->ResolveAssigns(PathList->Get<CString>(i)));
		}

		if (IOSrv->LoadFileToBuffer("Proj:SrcPathList.hrd", Buffer))
		{
			Data::PParams PathList;
			if (!Parser.ParseBuffer((LPCSTR)Buffer.GetPtr(), Buffer.GetSize(), PathList)) return ERR_IO_READ;

			if (PathList.IsValidPtr())
				for (int i = 0; i < PathList->GetCount(); ++i)
					IOSrv->SetAssign(PathList->Get(i).GetName().CStr(), IOSrv->ResolveAssigns(PathList->Get<CString>(i)));
		}
	}

	if (RootPath.IsEmpty()) RootPath = IOSrv->ResolveAssigns("Proj:Src/Shaders");
	else RootPath = IOSrv->ResolveAssigns(RootPath);
	RootPath.Trim();
	RootPath.Replace('\\', '/');
	if (RootPath[RootPath.GetLength() - 1] != '/') RootPath += '/';
	if (RootPath.IsEmpty()) return ExitApp(ERR_INVALID_CMD_LINE, WaitKey);

	if (DB.IsEmpty())
	{
		DB = RootPath;
		DB += "ShaderDB.db3";
	}
	else DB = IOSrv->ResolveAssigns(DB);

	if (Rebuild) IOSrv->DeleteFile(DB);
	if (!OpenDB(DB)) return ExitApp(ERR_MAIN_FAILED, WaitKey);

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

	for (int i = 0; i < InList.GetCount(); ++i)
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
			int Code = CompileEffect(InRec, OutRec, Debug);
			if (Code != 0) return ExitApp(Code, WaitKey);
		}
	}

	return ExitApp(SUCCESS, WaitKey);
}
//---------------------------------------------------------------------

int ExitApp(int Code, bool WaitKey)
{
	CloseDB();

	if (Code != SUCCESS) n_msg(VL_ERROR, TOOL_NAME" v"VERSION": Error occurred with code %d\n", Code);

	if (WaitKey)
	{
		Sys::Log("\nPress any key to exit...\n");
		_getch();
	}

	return Code;
}
//---------------------------------------------------------------------
