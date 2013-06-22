#include <IO/IOServer.h>
#include <ConsoleApp.h>

#define TOOL_NAME	"CFCopy"
#define VERSION		"1.0"

#define SUCCESS						0
#define ERR_COPY_FAILED				1
#define ERR_IN_OUT_TYPES_DONT_MATCH 2
#define ERR_IN_NOT_FOUND			3

int Verbose = VR_ERROR;

int ExitApp(int Code, bool WaitKey);

API int Run(int argc, const char** argv)
{
	nCmdLineArgs Args(argc, argv);

	bool WaitKey = Args.GetBoolArg("-waitkey");
	nString In = Args.GetStringArg("-in");
	nString Out = Args.GetStringArg("-out");
	Verbose = Args.GetIntArg("-v");

	Ptr<IO::CIOServer> IOServer = n_new(IO::CIOServer);

	bool Dir = IOSrv->DirectoryExists(In);
	if (!Dir && IOSrv->DirectoryExists(Out)) return ExitApp(ERR_IN_OUT_TYPES_DONT_MATCH, WaitKey);

	if (Dir)
		return ExitApp(IOSrv->CopyDirectory(In, Out, Args.GetBoolArg("-r")) ? SUCCESS : ERR_COPY_FAILED, WaitKey);
	else
	{
		if (!IOSrv->FileExists(In)) return ExitApp(ERR_IN_NOT_FOUND, WaitKey);
		return ExitApp(IOSrv->CopyFile(In, Out) ? SUCCESS : ERR_COPY_FAILED, WaitKey);
	}
}
//---------------------------------------------------------------------

int main(int argc, const char** argv)
{
	return Run(argc, argv);
}
//---------------------------------------------------------------------

int ExitApp(int Code, bool WaitKey)
{
	if (Code != SUCCESS) n_msg(VR_ERROR, TOOL_NAME" v"VERSION": Error occured with code %d\n", Code);
	if (WaitKey)
	{
		n_printf("\nPress any key to exit...\n");
		getch();
	}

	return Code;
}
//---------------------------------------------------------------------
