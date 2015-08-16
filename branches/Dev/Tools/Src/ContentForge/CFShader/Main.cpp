#include <IO/IOServer.h>
#include <Data/StringTokenizer.h>
#include <ConsoleApp.h>
#include <ShaderDB.h>

// Debug args:
// -waitkey -v 5 -root "..\..\..\..\..\InsanePoet\Content\Src\Shaders" -in "..\..\..\..\..\InsanePoet\Content\Src\Shaders\SM_4_0\CEGUI.hrd" -out "..\..\..\..\..\InsanePoet\Content\Export\Shaders\SM_4_0\CEGUI.eff"

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
				"-in [filepath{;filepath}]   input file(s)\n"
				"-out [filepath{;filepath}]  output file(s), count must be the same\n"
				"-root [path]                root path to shaders, typically the same as\n"
				"                            'Shaders:' assign\n"
				"-db [filepath]              path to persistent shader DB,\n"
				"                            default: -root + 'ShaderDB.db3'\n"
				"-d                          build shaders with debug info\n"
				"-waitkey                    wait for a key press when tool has finished\n"
				"-v [verbosity]              output verbosity level, [ 0 .. 5 ]\n");

		return ExitApp(SUCCESS_HELP, WaitKey);
	}

	const char* pIn = Args.GetStringArg("-in");
	const char* pOut = Args.GetStringArg("-out");
	RootPath = Args.GetStringArg("-root");
	CString DB(Args.GetStringArg("-db"));
	bool Debug = Args.GetBoolArg("-d");

	if (!pIn || !pOut || !*pIn || !*pOut) return ExitApp(ERR_INVALID_CMD_LINE, WaitKey);

	if (DB.IsEmpty())
	{
		DB = RootPath;
		DB.Trim();
		DB.Replace('\\', '/');
		if (DB.IsValid() && DB[DB.GetLength() - 1] != '/') DB += '/';
		DB += "ShaderDB.db3";
	}
	if (!OpenDB(DB)) return ExitApp(ERR_MAIN_FAILED, WaitKey);

	IO::CIOServer IOServer;

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
