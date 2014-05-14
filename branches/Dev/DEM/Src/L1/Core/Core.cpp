#include "Core.h"

#include <Data/String.h>

namespace Core
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
	va_list Args;
	va_start(Args, pMsg);
	Buffer.Format("*** DEM ASSERTION FAILED ***\nMessage: %s\nExpression: %s\nFile: %s\nLine: %d\nFunction: %s\n", pMsg, pExpression, pFile, Line, pFunc);
	va_end(Args);

	if (!pLogHandler) pLogHandler = DefaultLogHandler;
	(*pLogHandler)(MsgType_Error, Buffer.CStr());

	//!!!msgbox can have buttons to continue or fail!
	OK;
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
