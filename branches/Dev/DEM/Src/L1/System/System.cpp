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

	CString Buffer, MsgBoxBuffer;
	va_list Args;
	va_start(Args, pMsg);
	Buffer.Format("*** DEM ASSERTION FAILED ***\nMessage: %s\nExpression: %s\nFile: %s\nLine: %d\nFunction: %s\n", pMsg, pExpression, pFile, Line, pFunc);
	//???can here? or copy args?
	//???MsgBoxBuffer.Format("*** DEM ASSERTION FAILED ***\nMessage: %s\nExpression: %s\nFile: %s\nLine: %d\nFunction: %s\n", pMsg, pExpression, pFile, Line, pFunc);
	va_end(Args);

	char Trace[4096];
	Sys::TraceStack(Trace, sizeof(Trace));
	Buffer += "Call stack:\n";
	Buffer += Trace;

	if (!pLogHandler) pLogHandler = DefaultLogHandler;
	(*pLogHandler)(MsgType_Error, Buffer.CStr());

	Buffer.Clear();

	// Clamp text to fit into a message box
	Trace[2000] = 0;
	MsgBoxBuffer += "\nCall stack:\n";
	MsgBoxBuffer += Trace;

	//!!!clamp text in message box! ShowMessageBox() flag?
	//!!!need CString::SetLength/Truncate!
	//!!!can show message box here instead of handling it in a log handler
	//good effect is tha we can log one message and show other (formatting)
	return Sys::ShowMessageBox(MsgType_Error, NULL, MsgBoxBuffer.CStr(), MBB_OK | MBB_Cancel) == MBB_OK;
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
