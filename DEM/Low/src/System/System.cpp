#include "System.h"

#include <Data/String.h>
#include <crtdbg.h>

namespace Sys
{
static FLogHandler pLogHandler = nullptr;

extern void DefaultLogHandler(EMsgType Type, std::string_view Message);

void DebugBreak()
{
	__debugbreak();
}
//---------------------------------------------------------------------

void Crash(const char* pFile, int Line, std::string_view Message)
{
#ifdef _DEBUG
	const int CRTReportMode = _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_WNDW);
	_CrtSetReportMode(_CRT_ERROR, CRTReportMode);

	const int Result = _CrtDbgReport(_CRT_ERROR, pFile, Line, "DeusExMachina game engine", std::string(Message).c_str());
	if (Result == 0 && (CRTReportMode & _CRTDBG_MODE_WNDW))
		return;
	else if (Result == 1)
		_CrtDbgBreak();
#endif

	abort();
}
//---------------------------------------------------------------------

bool ReportAssertionFailure(const char* pExpression, std::string_view Message, const char* pFile, int Line, const char* pFunc)
{
	char TraceBuf[4096];
	const auto Trace = Sys::TraceStack(TraceBuf, sizeof(TraceBuf)) ? std::string_view(TraceBuf) : std::string_view{};

	const auto InfoBlockStr =
"Message:    {}\
Expression: {}\
File:       {}\
Line:       {}\
Function:   {}"_format(Message.empty() ? "none"sv : Message, pExpression, pFile, Line, pFunc);

	// Log
	{
		auto LogMsg = "*** DEM ASSERTION FAILED ***\n{}"_format(InfoBlockStr);
		if (!Trace.empty())
		{
			LogMsg.append("\nCall stack:\n");
			LogMsg.append(Trace);
		}

		if (!pLogHandler) pLogHandler = DefaultLogHandler;
		(*pLogHandler)(MsgType_Error, LogMsg);
	}

	// Message box
	{
		auto MsgBoxMsg =
			"*** DEM ASSERTION FAILED ***\
\
PRESS OK TO CONTINUE EXECUTION\
\
{}\n"_format(InfoBlockStr);
		if (!Trace.empty())
		{
			MsgBoxMsg.append("\nCall stack:\n");
			MsgBoxMsg.append(Trace);
		}

		const auto Result = Sys::ShowMessageBox(MsgType_Error, {}, MsgBoxMsg, MBB_OK | MBB_Cancel);

		return Result != MBB_OK;
	}
}
//---------------------------------------------------------------------

// Critical error, program will be closed
void Error(std::string_view Message)
{
	if (!pLogHandler) pLogHandler = DefaultLogHandler;
	(*pLogHandler)(MsgType_Error, Message);

	//!!!file and line must be passed here in context!
	Crash(__FILE__, __LINE__, Message);
}
//---------------------------------------------------------------------

void Log(std::string_view Message)
{
	if (!pLogHandler) pLogHandler = DefaultLogHandler;
	(*pLogHandler)(MsgType_Log, Message);
}
//---------------------------------------------------------------------

void DbgOut(std::string_view Message)
{
	if (!pLogHandler) pLogHandler = DefaultLogHandler;
	(*pLogHandler)(MsgType_DbgOut, Message);
}
//---------------------------------------------------------------------

void Message(std::string_view Message)
{
	if (!pLogHandler) pLogHandler = DefaultLogHandler;
	(*pLogHandler)(MsgType_Message, Message);
}
//---------------------------------------------------------------------

}
