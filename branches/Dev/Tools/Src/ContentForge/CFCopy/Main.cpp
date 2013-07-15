#include <IO/IOServer.h>
#include <Data/StringTokenizer.h>
#include <ConsoleApp.h>

#define TOOL_NAME	"CFCopy"
#define VERSION		"1.0"

int Verbose = VL_ERROR;

int ExitApp(int Code, bool WaitKey);

int main(int argc, const char** argv)
{
	nCmdLineArgs Args(argc, argv);

	bool WaitKey = Args.GetBoolArg("-waitkey");
	CString In = Args.GetStringArg("-in");
	CString Out = Args.GetStringArg("-out");
	Verbose = Args.GetIntArg("-v");

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

		n_msg(VL_INFO, "Copying pair %d: '%s' -> '%s'\n", i, In.CStr(), Out.CStr());

		bool Dir = IOSrv->DirectoryExists(In);
		if (!Dir && IOSrv->DirectoryExists(Out)) return ExitApp(ERR_IN_OUT_TYPES_DONT_MATCH, WaitKey);

		if (Dir)
		{
			if (!IOSrv->CopyDirectory(In, Out, Args.GetBoolArg("-r"))) return ExitApp(ERR_MAIN_FAILED, WaitKey);
		}
		else
		{
			if (!IOSrv->FileExists(In)) return ExitApp(ERR_IN_NOT_FOUND, WaitKey);
			IOSrv->CreateDirectory(Out.ExtractDirName());
			if (!IOSrv->CopyFile(In, Out)) return ExitApp(ERR_MAIN_FAILED, WaitKey);
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
