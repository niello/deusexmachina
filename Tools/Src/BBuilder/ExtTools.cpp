#include <IO/IOServer.h>
#include <Data/StringID.h>
#include <util/nstring.h>
//#include <util/ndictionary.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//nDictionary<CStrID, HMODULE> Modules;

//typedef int (*CToolRunProc)(int argc, const char** argv);

//int RunExternalToolAsFunc(CStrID Name, int argc, const char** argv)

// It has memory issues not solved yet:
/*
HMODULE hModule = LoadLibrary(Path.CStr());
if (!hModule) return -1;

CToolRunProc Run = (CToolRunProc)GetProcAddress(hModule, "Run");
int Result = Run ? Run(argc, argv) : -1;

FreeLibrary(hModule);
*/

int RunExternalToolAsProcess(CStrID Name, LPSTR pCmdLine, LPCSTR pWorkingDir)
{
	n_printf("> %s %s\n", Name.CStr(), pCmdLine);

	nString Path = IOSrv->ManglePath("home:");
	Path += "\\..\\ContentForge\\";
	Path += Name.CStr();
	Path +=	".exe";
	Path.Replace('/', '\\');

	PROCESS_INFORMATION Info;
	RtlZeroMemory(&Info, sizeof(Info));

	STARTUPINFO SUI;
	RtlZeroMemory(&SUI, sizeof(SUI));
	SUI.cb = sizeof(SUI);
	SUI.dwFlags = STARTF_FORCEOFFFEEDBACK;
	if (CreateProcess(Path.CStr(), pCmdLine, NULL, NULL, FALSE, 0, NULL, pWorkingDir, &SUI, &Info) == FALSE) return -1;

	int Result = -1;
	WaitForSingleObject(Info.hProcess, INFINITE);

	GetExitCodeProcess(Info.hProcess, (DWORD*)&Result);

	CloseHandle(Info.hThread);
	CloseHandle(Info.hProcess);

	return Result;
}
//---------------------------------------------------------------------
