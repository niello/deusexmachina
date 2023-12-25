#if DEM_PLATFORM_WIN32
#include <System/System.h>

#include <Data/String.h>
#include <IO/PathUtils.h>

#include <stdio.h>
#include <malloc.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>

// OS-specific implementations of some system functions

namespace Sys
{

EMsgBoxButton ShowMessageBox(EMsgType Type, const char* pHeaderText, const char* pMessage, unsigned int Buttons)
{
	// Find app window, and minimize it. This is necessary when in fullscreen mode, otherwise
	// the message box may not be visible.
	//???Use D3D device DialogBoxMode? or externally before calling this?
	HWND hWnd = ::GetForegroundWindow(); //???GetActiveWindow [ + GetParent()]?
	if (hWnd) ::ShowWindow(hWnd, SW_MINIMIZE);

	char HeaderBuf[256];
	if (!pHeaderText)
	{
		if (!hWnd) hWnd = ::GetActiveWindow();
		pHeaderText = (hWnd && ::GetWindowText(hWnd, HeaderBuf, sizeof(HeaderBuf)) > 0) ? HeaderBuf : "";
	}

	UINT BoxType = MB_APPLMODAL | MB_SETFOREGROUND | MB_TOPMOST;
	switch (Type)
	{
		case MsgType_Error:	BoxType |= MB_ICONERROR; break;
		default:			BoxType |= MB_ICONINFORMATION; break;
	}

	switch (Buttons)
	{
		case MBB_OK | MBB_Cancel:					BoxType |= MB_OKCANCEL; break;
		case MBB_Yes | MBB_No:						BoxType |= MB_YESNO; break;
		case MBB_Yes | MBB_No | MBB_Cancel:			BoxType |= MB_YESNOCANCEL; break;
		case MBB_Retry | MBB_Cancel:				BoxType |= MB_RETRYCANCEL; break;
		case MBB_Abort | MBB_Retry | MBB_Ignore:	BoxType |= MB_ABORTRETRYIGNORE; break;
		default:									BoxType |= MB_OK; break;
	}

	int Ret = ::MessageBox(nullptr, pMessage, pHeaderText, BoxType);

	switch (Ret)
	{
		case IDOK:		return MBB_OK;
		case IDABORT:	return MBB_Abort;
		case IDRETRY:	return MBB_Retry;
		case IDIGNORE:	return MBB_Ignore;
		case IDYES:		return MBB_Yes;
		case IDNO:		return MBB_No;
		case IDCANCEL:
		default:		return MBB_Cancel;
	}
}
//---------------------------------------------------------------------

//!!!???to Thread namespace/class?!
void Sleep(unsigned long MSec)
{
	::Sleep(MSec);
}
//---------------------------------------------------------------------

void SetCurrentThreadName(std::string_view Name)
{
	const std::wstring WName(Name.cbegin(), Name.cend());
	::SetThreadDescription(::GetCurrentThread(), WName.c_str());
}
//---------------------------------------------------------------------

bool GetKeyName(U8 ScanCode, bool ExtendedKey, CString& OutName)
{
	//???build DEM KeyCode -> Key name table?
	LONG lParam = (ScanCode << 16);
	if (ExtendedKey) lParam |= (1 << 24);
	char Buffer[512];
	int Len = ::GetKeyNameText(lParam, Buffer, sizeof(Buffer));
	if (!Len) FAIL;
	OutName.Set(Buffer, Len);
	OK;
}
//---------------------------------------------------------------------

double GetAppTime()
{
	LONGLONG PerfTime, PerfFreq;
	QueryPerformanceCounter((LARGE_INTEGER*)&PerfTime);
	QueryPerformanceFrequency((LARGE_INTEGER*)&PerfFreq);
	return PerfTime / (double)PerfFreq;
}
//---------------------------------------------------------------------

//!!!???to System.cpp, implement single Win32 calls as Sys:: functions here!
void DefaultLogHandler(EMsgType Type, const char* pMessage) //!!!context, see Qt!
{
	//!!!see Qt: QString logMessage = qMessageFormatString(type, context, buf);!

	switch (Type)
	{
		case MsgType_Message:
		{
			ShowMessageBox(Type, nullptr, pMessage);
			// Fallback to MsgType_Log intentionally
		}
		case MsgType_Log:
		{
			HANDLE hStdOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
			if (hStdOut && hStdOut != INVALID_HANDLE_VALUE)
			{
				::WriteFile(hStdOut, pMessage, strlen(pMessage), nullptr, nullptr);
				::FlushFileBuffers(hStdOut); //PERF //???always? isn't too slow?
				return;
			}
			break;
		}
		case MsgType_Error:
		{
			HANDLE hStdErr = ::GetStdHandle(STD_ERROR_HANDLE);
			if (hStdErr && hStdErr != INVALID_HANDLE_VALUE)
			{
				::WriteFile(hStdErr, pMessage, strlen(pMessage), nullptr, nullptr);
				::FlushFileBuffers(hStdErr);
				return;
			}
			break;
		}
	}

	// DbgOut and any messages not handled
	OutputDebugString(pMessage);
}
//---------------------------------------------------------------------

struct CStackTraceUserCtx
{
	char*			pTrace;
	char*			pOut;
	unsigned int	MaxLength;
	DWORD64			Base;
};

static BOOL CALLBACK EnumSymbolsCallback(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID pUserContext)
{
	if (pSymInfo->Flags & IMAGEHLP_SYMBOL_INFO_PARAMETER)
	{
		CStackTraceUserCtx* pUserCtx = (CStackTraceUserCtx*)pUserContext;
		pUserCtx->pOut += _snprintf_s(
			pUserCtx->pOut,
			pUserCtx->pTrace + pUserCtx->MaxLength - 1 - pUserCtx->pOut,
			_TRUNCATE,
			"%s=0x%x ",
			pSymInfo->Name, *(DWORD*)(pUserCtx->Base + pSymInfo->Address - 8));
	}
	return true;
}
//---------------------------------------------------------------------

bool TraceStack(char* pTrace, unsigned int MaxLength)
{
	if (!pTrace || !MaxLength) FAIL;

	*pTrace = 0;

	HANDLE hProcess = ::GetCurrentProcess();
	HANDLE hThread = ::GetCurrentThread();
	CONTEXT ThreadCtx;
	ZeroMemory(&ThreadCtx, sizeof(ThreadCtx));
	ThreadCtx.ContextFlags = CONTEXT_FULL;

	// Works since WinXP
	::RtlCaptureContext(&ThreadCtx);

	// Add executable's directory to the search path in case working dir is not exe dir
	char PathBuf[DEM_MAX_PATH];
	::GetModuleFileName(0,  PathBuf, sizeof(PathBuf));
	for (int i = DEM_MAX_PATH - 1; i >= 0; --i)
	{
		if (PathBuf[i] == '\\' || PathBuf[i] == '/')
		{
			PathBuf[i] = 0;
			break;
		}
	}
	if (!::SymInitialize(hProcess, (PCSTR)PathBuf, true)) FAIL;

	STACKFRAME64 Frame = { 0 };
	Frame.AddrPC.Mode = AddrModeFlat;
	Frame.AddrStack.Mode = AddrModeFlat;
	Frame.AddrFrame.Mode = AddrModeFlat;

#if _M_IX86
	Frame.AddrPC.Offset = ThreadCtx.Eip;
	Frame.AddrStack.Offset = ThreadCtx.Esp;
	Frame.AddrFrame.Offset = ThreadCtx.Ebp;
	constexpr DWORD Arch = IMAGE_FILE_MACHINE_I386;
#elif _M_X64
	Frame.AddrPC.Offset = ThreadCtx.Rip;
	Frame.AddrStack.Offset = ThreadCtx.Rsp;
	Frame.AddrFrame.Offset = ThreadCtx.Rbp;
	constexpr DWORD Arch = IMAGE_FILE_MACHINE_AMD64;
#endif

	char* pOut = pTrace;

	const int MAX_NAME_LEN = 512;
	SYMBOL_INFO* pSymInfo = (SYMBOL_INFO*)n_malloc(sizeof(SYMBOL_INFO) + MAX_NAME_LEN);

	while (::StackWalk64(
				Arch,
				hProcess,
				hThread,
				&Frame,
				&ThreadCtx,
				nullptr,
				::SymFunctionTableAccess64,
				::SymGetModuleBase64,
				nullptr))
	{
		ZeroMemory(pSymInfo, sizeof(SYMBOL_INFO) + MAX_NAME_LEN);
		pSymInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
		pSymInfo->MaxNameLen = MAX_NAME_LEN;

		if (Frame.AddrPC.Offset)
		{
			DWORD64 Offset;
			if (::SymFromAddr(hProcess, Frame.AddrPC.Offset, &Offset, pSymInfo))
			{
				pOut += _snprintf_s(pOut, MaxLength - 1 + pTrace - pOut, _TRUNCATE, "%s(", pSymInfo->Name);

				// Show locals
				IMAGEHLP_STACK_FRAME SF;
				ZeroMemory(&SF, sizeof(SF));
				SF.InstructionOffset    = Frame.AddrPC.Offset;
				SF.ReturnOffset         = Frame.AddrReturn.Offset;
				SF.FrameOffset          = Frame.AddrFrame.Offset;
				SF.StackOffset          = Frame.AddrStack.Offset;
				SF.BackingStoreOffset   = Frame.AddrBStore.Offset;
				SF.FuncTableEntry       = (ULONG64)Frame.FuncTableEntry;
				SF.Params[0]            = Frame.Params[0];
				SF.Params[1]            = Frame.Params[1];
				SF.Params[2]            = Frame.Params[2];
				SF.Params[3]            = Frame.Params[3];
				SF.Virtual              = Frame.Virtual;

				if (::SymSetContext(hProcess, &SF, 0) == TRUE || GetLastError() == ERROR_SUCCESS)
				{
					CStackTraceUserCtx UserCtx = { pTrace, pOut, MaxLength, Frame.AddrStack.Offset };
					if (::SymEnumSymbols(hProcess, 0, "[a-zA-Z0-9_]*", EnumSymbolsCallback, &UserCtx))
						pOut = UserCtx.pOut;
					else pOut += _snprintf_s(pOut, MaxLength - 1 + pTrace - pOut, _TRUNCATE, "no symbols available");
				}

				pOut += _snprintf_s(pOut, MaxLength - 1 + pTrace - pOut, _TRUNCATE, ")\n");
			}
			else pOut += _snprintf_s(pOut, MaxLength - 1 + pTrace - pOut, _TRUNCATE, "(no symbol name)\n");

			// Source code line
			IMAGEHLP_LINE64 Line;
			ZeroMemory(&Line, sizeof(Line));
			Line.SizeOfStruct = sizeof(Line);
			DWORD LineOffset;
			if (::SymGetLineFromAddr64(hProcess, Frame.AddrPC.Offset, &LineOffset, &Line))
				pOut += _snprintf_s(pOut, MaxLength - 1 + pTrace - pOut, _TRUNCATE, "<%s, %d>\n", Line.FileName, Line.LineNumber);
		}
		else pOut += _snprintf_s(pOut, MaxLength - 1 + pTrace - pOut, _TRUNCATE, "Error: EIP=0\n");
	}

	n_free(pSymInfo);

	return ::SymCleanup(hProcess) == TRUE;
}
//---------------------------------------------------------------------

}

#endif
