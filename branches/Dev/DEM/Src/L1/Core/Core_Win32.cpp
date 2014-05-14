#ifdef __WIN32__

#include "Core.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// OS-specific implementations of some core functions

namespace Core
{

void ShowMessageBox(EMsgType Type, const char* pHeaderText, const char* pMessage)
{
	// Find app window, and minimize it. This is necessary when in fullscreen mode, otherwise
	// the message box may not be visible.
	//???Use D3D device DialogBoxMode? or externally before calling this?
	HWND hWnd = GetForegroundWindow(); //???GetActiveWindow [ + GetParent()]?
	if (hWnd) ShowWindow(hWnd, SW_MINIMIZE);

	char HeaderBuf[256];
	if (!pHeaderText)
	{
		if (!hWnd) hWnd = GetActiveWindow();
		pHeaderText = (hWnd && GetWindowText(hWnd, HeaderBuf, sizeof(HeaderBuf)) > 0) ? HeaderBuf : "";
	}

	UINT BoxType = MB_OK | MB_APPLMODAL | MB_SETFOREGROUND | MB_TOPMOST;
	switch (Type)
	{
		case MsgType_Error:	BoxType |= MB_ICONERROR; break;
		default:			BoxType |= MB_ICONINFORMATION; break;
	}

	MessageBox(NULL, pMessage, pHeaderText, BoxType);
}
//---------------------------------------------------------------------

//!!!???to Thread namespace/class?!
void Sleep(unsigned long MSec)
{
	::Sleep(MSec);
}
//---------------------------------------------------------------------

void DefaultLogHandler(EMsgType Type, const char* pMessage) //!!!context, see Qt!
{
	//!!!see Qt: QString logMessage = qMessageFormatString(type, context, buf);!

	switch (Type)
	{
		case MsgType_Message:
		{
			ShowMessageBox(Type, NULL, pMessage);
			// Fallback to Log intentionally
		}
		case MsgType_Log:
		{
			HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
			if (hStdOut && hStdOut != INVALID_HANDLE_VALUE)
			{
				WriteFile(hStdOut, pMessage, strlen(pMessage), NULL, NULL);
				FlushFileBuffers(hStdOut); //PERF //???always? isn't too slow?
				return;
			}
			break;
		}
		case MsgType_Error:
		{
			ShowMessageBox(Type, NULL, pMessage);

			HANDLE hStdErr = GetStdHandle(STD_ERROR_HANDLE);
			if (hStdErr && hStdErr != INVALID_HANDLE_VALUE)
			{
				WriteFile(hStdErr, pMessage, strlen(pMessage), NULL, NULL);
				FlushFileBuffers(hStdErr);
				return;
			}

			break;
		}
	}

	// DbgOut and any messages not handled
	OutputDebugString(pMessage);
}
//---------------------------------------------------------------------

}

#endif