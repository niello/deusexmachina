#include "Logger.h"

#include <IO/IOServer.h>
#include <IO/Streams/FileStream.h>
#include <Events/EventServer.h>
#include <Render/RenderServer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#undef CreateDirectory

#define MAX_MSG_SIZE 8192 //!!!trims long messages like shader compiler output!

namespace Core
{
__ImplementSingleton(CLogger);

bool CLogger::Open(const char* pAppName, const CString& FilePath)
{
	AppName = pAppName;

	n_assert(!pLogFile && !_IsOpen);
	_IsOpen = true;

	CString Dir;
	CString RealPath(FilePath);
	if (RealPath.IsEmpty())
	{
		Dir = "AppData:STILL NO TEAM NAME/Logs";
		RealPath.Format("AppData:STILL NO TEAM NAME/Logs/%s.log", AppName.CStr());
	}
	else Dir = RealPath.ExtractDirName();

	if (!IOSrv->DirectoryExists(Dir) && !IOSrv->CreateDirectory(Dir)) FAIL;

	pLogFile = n_new(IO::CFileStream);

	// Note: Failing to open the log file is not an error. There may
	// be several versions of the same application running, which
	// would fight for the log file. The first one wins, the other
	// silently don't log.
	pLogFile->Open(RealPath, IO::SAM_WRITE, IO::SAP_SEQUENTIAL);
	OK;
}
//---------------------------------------------------------------------

void CLogger::Close()
{
	n_assert(_IsOpen && pLogFile);
	n_delete(pLogFile);
	pLogFile = NULL;
	_IsOpen = false;
}
//---------------------------------------------------------------------

void CLogger::PrintInternal(char* pOutStr, int BufLen, EMsgType Type, const char* pMsg, va_list Args)
{
	if (!pMsg || !pOutStr) return;

	_vsnprintf_s(pOutStr, BufLen, BufLen - 1, pMsg, Args);
	pOutStr[BufLen - 1] = 0;
	LineBuffer.Put(pOutStr);

	if (pLogFile && pLogFile->IsOpen()) pLogFile->Write(pOutStr, strlen(pOutStr));
	if (Events::CEventServer::HasInstance())
	{
		Data::PParams P = n_new(Data::CParams(2));
		P->Set(CStrID("pMsg"), (PVOID)pOutStr);
		P->Set(CStrID("Type"), (int)Type);
		EventSrv->FireEvent(CStrID("OnLogMsg"), P); //???or add direct listeners to log handler?
	}
}
//---------------------------------------------------------------------

void CLogger::Print(const char* pMsg, va_list Args)
{
	char pLine[MAX_MSG_SIZE];
	PrintInternal(pLine, MAX_MSG_SIZE, MsgTypeMessage, pMsg, Args);
}
//---------------------------------------------------------------------

void CLogger::Message(const char* pMsg, va_list Args)
{
	char pLine[MAX_MSG_SIZE];
	PrintInternal(pLine, MAX_MSG_SIZE, MsgTypeMessage, pMsg, Args);
	ShowMessageBox(MsgTypeMessage, pLine);
}
//---------------------------------------------------------------------

void CLogger::Error(const char* pMsg, va_list Args)
{
	char pLine[MAX_MSG_SIZE];
	PrintInternal(pLine, MAX_MSG_SIZE, MsgTypeError, pMsg, Args);
	ShowMessageBox(MsgTypeError, pLine);
	CLogger::~CLogger(); // Kill self on error
}
//---------------------------------------------------------------------

void CLogger::OutputDebug(const char* pMsg, va_list Args)
{
	char pLine[MAX_MSG_SIZE];
	PrintInternal(pLine, MAX_MSG_SIZE, MsgTypeMessage, pMsg, Args);
	OutputDebugString(pLine);
}
//---------------------------------------------------------------------

//!!!it is very stupid placing this stuff in logger! redesign logger!
void CLogger::ShowMessageBox(EMsgType Type, const char* pMsg)
{
	// Find app window, and minimize it. This is necessary when in Fullscreen mode, otherwise
	// the MessageBox() may not be visible.
	//???Use D3D device DialogBoxMode?
	if (Type == MsgTypeError)
	{
		HWND hWnd = RenderSrv->GetDisplay().GetAppHwnd();
		if (hWnd) ShowWindow(hWnd, SW_MINIMIZE);
	}

	UINT BoxType = (MB_OK | MB_APPLMODAL | MB_SETFOREGROUND | MB_TOPMOST);
	switch (Type)
	{
		case MsgTypeMessage:	BoxType |= MB_ICONINFORMATION; break;
		case MsgTypeError:		BoxType |= MB_ICONERROR; break;
	}

	MessageBox(NULL, pMsg, AppName.CStr(), BoxType);
}
//---------------------------------------------------------------------

}
