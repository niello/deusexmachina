#include "System.h"

#include <Data/String.h>

namespace Sys
{
static FLogHandler pLogHandler = NULL;

extern void DefaultLogHandler(EMsgType Type, const char* pMessage);

void Crash(const char* pFile, int Line, const char* pMessage)
{
	int CRTReportMode = _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_WNDW);
	_CrtSetReportMode(_CRT_ERROR, CRTReportMode);

	int Result = _CrtDbgReport(_CRT_ERROR, pFile, Line, "DeusExMachina game engine", pMessage);
	if (Result == 0 && CRTReportMode & _CRTDBG_MODE_WNDW) return;
	else if (Result == 1) _CrtDbgBreak();

	abort();
}
//---------------------------------------------------------------------

bool ReportAssertionFailure(const char* pExpression, const char* pMessage, const char* pFile, int Line, const char* pFunc)
{
	const char* pMsg = pMessage ? pMessage : "none";

	CString Buffer;
	Buffer.Format("*** DEM ASSERTION FAILED ***\nMessage:    %s\nExpression: %s\nFile:       %s\nLine:       %d\nFunction:   %s\n", pMsg, pExpression, pFile, Line, pFunc);

	char Trace[4096];
	if (Sys::TraceStack(Trace, sizeof(Trace)))
	{
		Buffer += "Call stack:\n";
		Buffer += Trace;
	}

	if (!pLogHandler) pLogHandler = DefaultLogHandler;
	(*pLogHandler)(MsgType_Error, Buffer.CStr());

	Buffer.Format("*** DEM ASSERTION FAILED ***\n\nPRESS OK TO CONTINUE EXECUTION\n\nMessage: %s\nExpression: %s\nFile: %s\nLine: %d\nFunction: %s\n", pMsg, pExpression, pFile, Line, pFunc);

	// Clamp text to fit into a message box
	//!!!need CString::SetLength/Truncate!
	Trace[1600] = 0;
	if (Trace[0])
	{
		Buffer += "\nCall stack:\n";
		Buffer += Trace;
	}

	return Sys::ShowMessageBox(MsgType_Error, NULL, Buffer.CStr(), MBB_OK | MBB_Cancel) != MBB_OK;
}
//---------------------------------------------------------------------

// Critical error, program will be closed
void __cdecl Error(const char* pMsg, ...)
{
	CString Buffer;
	va_list Args;
	va_start(Args, pMsg);
	Buffer.FormatWithArgs(pMsg, Args);
	va_end(Args);

	if (!pLogHandler) pLogHandler = DefaultLogHandler;
	(*pLogHandler)(MsgType_Error, Buffer.CStr());

	//!!!file and line must be passed here in context!
	Crash(__FILE__, __LINE__, Buffer.CStr());
}
//---------------------------------------------------------------------

void __cdecl Log(const char* pMsg, ...)
{
	CString Buffer;
	va_list Args;
	va_start(Args, pMsg);
	Buffer.FormatWithArgs(pMsg, Args);
	va_end(Args);

	if (!pLogHandler) pLogHandler = DefaultLogHandler;
	(*pLogHandler)(MsgType_Log, Buffer.CStr());
}
//---------------------------------------------------------------------

void __cdecl DbgOut(const char* pMsg, ...)
{
	CString Buffer;
	va_list Args;
	va_start(Args, pMsg);
	Buffer.FormatWithArgs(pMsg, Args);
	va_end(Args);

	if (!pLogHandler) pLogHandler = DefaultLogHandler;
	(*pLogHandler)(MsgType_DbgOut, Buffer.CStr());
}
//---------------------------------------------------------------------

void __cdecl Message(const char* pMsg, ...)
{
	CString Buffer;
	va_list Args;
	va_start(Args, pMsg);
	Buffer.FormatWithArgs(pMsg, Args);
	va_end(Args);

	if (!pLogHandler) pLogHandler = DefaultLogHandler;
	(*pLogHandler)(MsgType_Message, Buffer.CStr());
}
//---------------------------------------------------------------------

}
