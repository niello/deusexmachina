#include "Main.h"

#include <IO/IOServer.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void BatchToolInOut(CStrID Name, const CString& InStr, const CString& OutStr)
{
	//???insert sorted?
	CArray<CString>& InList = InFileLists.GetOrAdd(Name);
	InList.Add(InStr);
	CArray<CString>& OutList = OutFileLists.GetOrAdd(Name);
	OutList.Add(OutStr);
}
//---------------------------------------------------------------------

int RunExternalToolAsProcess(CStrID Name, char* pCmdLine, const char* pWorkingDir)
{
	n_msg(VL_DETAILS, "> %s %s\n", Name.CStr(), pCmdLine);

	CString Path = IOSrv->ResolveAssigns("Home:");
	Path += "\\..\\ContentForge\\";
	Path += Name.CStr();
	Path +=	".exe";
	Path.Replace('/', '\\');

	PROCESS_INFORMATION Info;
	RtlZeroMemory(&Info, sizeof(Info));

	STARTUPINFO SUI;
	RtlZeroMemory(&SUI, sizeof(SUI));
	SUI.cb = sizeof(SUI);
	SUI.dwFlags = STARTF_FORCEOFFFEEDBACK; // | STARTF_USESTDHANDLES
	//SUI.hStdOutput

	if (CreateProcess(Path.CStr(), pCmdLine, NULL, NULL, FALSE, 0, NULL, pWorkingDir, &SUI, &Info) == FALSE) return -1;

	int Result = -1;
	WaitForSingleObject(Info.hProcess, INFINITE);

	GetExitCodeProcess(Info.hProcess, (DWORD*)&Result);

	CloseHandle(Info.hThread);
	CloseHandle(Info.hProcess);

	return Result;
}
//---------------------------------------------------------------------

int RunExternalToolBatch(CStrID Tool, int Verb, const char* pExtraCmdLine, const char* pWorkingDir)
{
	int Idx = InFileLists.FindIndex(Tool);
	if (Idx == INVALID_INDEX) return 0;
	CArray<CString>& InList = InFileLists.ValueAt(Idx);

	Idx = OutFileLists.FindIndex(Tool);
	if (Idx == INVALID_INDEX) return 0;
	CArray<CString>& OutList = OutFileLists.ValueAt(Idx);

	if (InList.GetCount() != OutList.GetCount()) return -1;
	if (InList.GetCount() == 0) return 0;

	for (UPTR i = 0; i < InList.GetCount(); ++i)
	{
		InList[i] = IOSrv->ResolveAssigns(InList[i]);
		OutList[i] = IOSrv->ResolveAssigns(OutList[i]);
		//!!!GetRelativePath(Base) to reduce cmd line size!
		//!!!don't forget to pass BasePath or override working directory in that case!
		//!!!???use Curr/Overridden Working Directory as Base?!
	}

	CString InStr = InList[0], OutStr = OutList[0];

	for (UPTR i = 1; i < InList.GetCount(); ++i)
	{
		DWORD NextLength = 32 + InStr.GetLength() + OutStr.GetLength() + InList[i].GetLength() + OutList[i].GetLength();
		if (NextLength >= MAX_CMDLINE_CHARS)
		{
			if (InStr.FindIndex(' ') != INVALID_INDEX) InStr = "\"" + InStr + "\"";
			if (OutStr.FindIndex(' ') != INVALID_INDEX) OutStr = "\"" + OutStr + "\"";

			char CmdLine[MAX_CMDLINE_CHARS];
			sprintf_s(CmdLine, "-v %d -in %s -out %s %s", Verb, InStr.CStr(), OutStr.CStr(), pExtraCmdLine ? pExtraCmdLine : "");
			int ExitCode = RunExternalToolAsProcess(Tool, CmdLine, pWorkingDir);
			if (ExitCode != 0) return ExitCode;

			InStr = InList[i];
			OutStr = OutList[i];
		}
		else
		{
			InStr.Add(';');
			InStr += InList[i];
			OutStr.Add(';');
			OutStr += OutList[i];
		}
	}

	if (InStr.FindIndex(' ') != INVALID_INDEX) InStr = "\"" + InStr + "\"";
	if (OutStr.FindIndex(' ') != INVALID_INDEX) OutStr = "\"" + OutStr + "\"";

	char CmdLine[MAX_CMDLINE_CHARS];
	sprintf_s(CmdLine, "-v %d -in %s -out %s %s", Verb, InStr.CStr(), OutStr.CStr(), pExtraCmdLine ? pExtraCmdLine : "");
	return RunExternalToolAsProcess(Tool, CmdLine, pWorkingDir);
}
//---------------------------------------------------------------------
