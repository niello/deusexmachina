#include "Subprocess.h"
#include <Logging.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int RunSubprocess(const std::string& CommandLine, CThreadSafeLog* pLog)
{
	std::string CommandLineCopy = CommandLine;
	PROCESS_INFORMATION ProcessInformation = { 0 };
	STARTUPINFO StartupInfo = { 0 };
	StartupInfo.cb = sizeof(StartupInfo);

	if (!::CreateProcess(NULL, CommandLineCopy.data(), NULL, NULL, FALSE, 0, NULL, NULL, &StartupInfo, &ProcessInformation))
	{
		if (pLog)
		{
			char* pMsgBuf = nullptr;
			const DWORD ErrorCode = ::GetLastError();
			::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, ErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&pMsgBuf, 0, NULL);

			pLog->LogError(
				std::string("CreateProcess failed with error: ") + pMsgBuf + "\nCommand line: " + CommandLine);

			// Free resources created by the system
			::LocalFree(pMsgBuf);
		}

		return -1;
	}

	if (pLog) pLog->LogInfo("RunSubprocess: " + CommandLine);

	::WaitForSingleObject(ProcessInformation.hProcess, INFINITE);

	DWORD ExitCode;
	if (!::GetExitCodeProcess(ProcessInformation.hProcess, &ExitCode))
	{
		if (pLog) pLog->LogWarning("Can't obtain exit code of the subprocess!");
		ExitCode = -1;
	}

	CloseHandle(ProcessInformation.hThread);
	CloseHandle(ProcessInformation.hProcess);

	return ExitCode;
}
