#include <IO/IOServer.h>
#include <Render/D3DXDEMInclude.h>
#include <Data/StringTokenizer.h>
#include <ConsoleApp.h>

#define TOOL_NAME	"CFShader"
#define VERSION		"1.0"

int		Verbose = VL_ERROR;
CString	RootPath;

int		ExitApp(int Code, bool WaitKey);
int		CompileShader(const CString& InFilePath, const CString& OutFilePath, bool Debug, int OptimizationLevel);

int main(int argc, const char** argv)
{
	nCmdLineArgs Args(argc, argv);

	bool WaitKey = Args.GetBoolArg("-waitkey");
	Verbose = Args.GetIntArg("-v");
	CString In = Args.GetStringArg("-in");
	CString Out = Args.GetStringArg("-out");
	RootPath = Args.GetStringArg("-root");
	int OptLevel = Args.GetIntArg("-o");
	bool Debug = Args.GetBoolArg("-d");

	if (In.IsEmpty() || Out.IsEmpty()) return ExitApp(ERR_INVALID_CMD_LINE, WaitKey);

	Ptr<IO::CIOServer> IOServer = n_new(IO::CIOServer);

	CArray<CString> InList, OutList;

	{
		Data::CStringTokenizer StrTok(In.CStr(), ";");
		while (StrTok.GetNextTokenSingleChar())
			InList.Add(StrTok.GetCurrToken());
	}

	{
		Data::CStringTokenizer StrTok(Out.CStr(), ";");
		while (StrTok.GetNextTokenSingleChar())
			OutList.Add(StrTok.GetCurrToken());
	}

	if (InList.GetCount() != OutList.GetCount()) return ExitApp(ERR_INVALID_CMD_LINE, WaitKey);

	for (int i = 0; i < InList.GetCount(); ++i)
	{
		In = InList[i];
		Out = OutList[i];

		n_msg(VL_INFO, "Compiling pair %d: '%s' -> '%s'\n", i, In.CStr(), Out.CStr());

		bool Dir = IOSrv->DirectoryExists(In);
		if (!Dir && IOSrv->DirectoryExists(Out)) return ExitApp(ERR_IN_OUT_TYPES_DONT_MATCH, WaitKey);

		if (Dir)
		{
			return ExitApp(ERR_NOT_IMPLEMENTED_YET, WaitKey);
		}
		else
		{
			if (!IOSrv->FileExists(In)) return ExitApp(ERR_IN_NOT_FOUND, WaitKey);
			int Code = CompileShader(In, Out, Debug, OptLevel);
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
		n_printf("\nPress any key to exit...\n");
		getch();
	}

	return Code;
}
//---------------------------------------------------------------------
