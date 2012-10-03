#include "Logger.h"

#include <Data/DataServer.h>
#include <Data/Streams/FileStream.h>
#include <Events/EventManager.h>
#include <kernel/nkernelserver.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#undef CreateDirectory

namespace Core
{
__ImplementSingleton(CLogger);

bool CLogger::Open(const char* pAppName, const nString& FilePath)
{
	AppName = pAppName;

	n_assert(!pLogFile && !_IsOpen);
	_IsOpen = true;

	nString Dir;
	nString RealPath(FilePath);
	if (RealPath.IsEmpty())
	{
		Dir = "appdata:RadonLabs/Nebula2";
		RealPath.Format("appdata:RadonLabs/Nebula2/%s.log", AppName.Get());
	}
	else Dir = RealPath.ExtractDirName();

	if (!DataSrv->DirectoryExists(Dir) && !DataSrv->CreateDirectory(Dir)) FAIL;

	pLogFile = n_new(Data::CFileStream);

	// Note: Failing to open the log file is not an error. There may
	// be several versions of the same application running, which
	// would fight for the log file. The first one wins, the other
	// silently don't log.
	pLogFile->Open(RealPath, Data::SAM_WRITE, Data::SAP_SEQUENTIAL);
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

	vsnprintf(pOutStr, BufLen - 1, pMsg, Args);
	LineBuffer.Put(pOutStr);

	if (pLogFile && pLogFile->IsOpen()) pLogFile->Write(pOutStr, strlen(pOutStr));
	if (Events::CEventManager::HasInstance())
	{
		Data::PParams P = n_new(Data::CParams(2));
		P->Set(CStrID("pMsg"), (PVOID)pOutStr);
		P->Set(CStrID("Type"), (int)Type);
		EventMgr->FireEvent(CStrID("OnLogMsg"), P); //???or add direct listeners to log handler?
	}
}
//---------------------------------------------------------------------

void CLogger::Print(const char* pMsg, va_list Args)
{
	char pLine[2048];
	PrintInternal(pLine, 2048, MsgTypeMessage, pMsg, Args);
}
//---------------------------------------------------------------------

void CLogger::Message(const char* pMsg, va_list Args)
{
	char pLine[2048];
	PrintInternal(pLine, 2048, MsgTypeMessage, pMsg, Args);
	ShowMessageBox(MsgTypeMessage, pLine);
}
//---------------------------------------------------------------------

void CLogger::Error(const char* pMsg, va_list Args)
{
	char pLine[2048];
	PrintInternal(pLine, 2048, MsgTypeError, pMsg, Args);
	ShowMessageBox(MsgTypeError, pLine);
	CLogger::~CLogger(); // Kill self on error
}
//---------------------------------------------------------------------

void CLogger::OutputDebug(const char* pMsg, va_list Args)
{
	char pLine[2048];
	PrintInternal(pLine, 2048, MsgTypeMessage, pMsg, Args);
	OutputDebugString(pLine);
}
//---------------------------------------------------------------------

void CLogger::ShowMessageBox(EMsgType Type, const char* pMsg)
{
	// Find app window, and minimize it. This is necessary when in Fullscreen mode, otherwise
	// the MessageBox() may not be visible.
	//???Use DialogBoxMode?
	if (Type == MsgTypeError)
	{
		HWND hWnd = NULL;
		if (CoreSrv->GetGlobal("hwnd", (int&)hWnd) && hWnd) ShowWindow(hWnd, SW_MINIMIZE);
	}

	UINT BoxType = (MB_OK | MB_APPLMODAL | MB_SETFOREGROUND | MB_TOPMOST);
	switch (Type)
	{
		case MsgTypeMessage:	BoxType |= MB_ICONINFORMATION; break;
		case MsgTypeError:		BoxType |= MB_ICONERROR; break;
	}

	MessageBox(0, pMsg, AppName.Get(), BoxType);
}
//---------------------------------------------------------------------

}
