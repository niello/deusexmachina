#include <IO/IOServer.h>
#include <Data/StringTokenizer.h>
#include <ConsoleApp.h>

// Debug args:
// -waitkey -v 5 -root "..\..\..\..\..\InsanePoet\Content" -in "..\..\..\..\..\InsanePoet\Content\Src\Shaders\SM_4_0\CEGUI.hrd" -out "..\..\..\..\..\InsanePoet\Content\Export\Shaders\SM_4_0\CEGUI.eff"

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
	const char* pIn = Args.GetStringArg("-in");
	const char* pOut = Args.GetStringArg("-out");
	RootPath = Args.GetStringArg("-root");
	bool Debug = Args.GetBoolArg("-d");

	if (!pIn || !pOut || !*pIn || !*pOut) return ExitApp(ERR_INVALID_CMD_LINE, WaitKey);

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
	if (Code != SUCCESS) n_msg(VL_ERROR, TOOL_NAME" v"VERSION": Error occured with code %d\n", Code);

	if (WaitKey)
	{
		Sys::Log("\nPress any key to exit...\n");
		_getch();
	}

	return Code;
}
//---------------------------------------------------------------------
