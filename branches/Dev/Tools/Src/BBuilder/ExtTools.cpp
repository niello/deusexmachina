#include <IO/IOServer.h>
#include <Data/StringID.h>
#include <util/nstring.h>
#include <ConsoleApp.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int RunExternalToolAsProcess(CStrID Name, LPSTR pCmdLine, LPCSTR pWorkingDir)
{
	n_msg(VR_DETAILS, "> %s %s\n", Name.CStr(), pCmdLine);

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
