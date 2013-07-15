#pragma once
#ifndef __DEM_L1_LOGGER_H__
#define __DEM_L1_LOGGER_H__

#include <Core/Singleton.h>
#include <Data/LineBuffer.h>
#include <Data/String.h>

namespace IO
{
	class CFileStream;
}

namespace Core
{
#define CoreLogger			Core::CLogger::Instance()
#define CoreLoggerExists	Core::CLogger::HasInstance()

class CLogger
{
	__DeclareSingleton(CLogger);

public:

	enum EMsgType
	{
		MsgTypeMessage,
		MsgTypeError
	};

private:

	bool				_IsOpen;
	CString				AppName;
	IO::CFileStream*	pLogFile;
	CLineBuffer			LineBuffer;

	//void PutLineBuffer(const char* msg, va_list argList);
	void PrintInternal(char* pOutStr, int BufLen, EMsgType Type, const char* pMsg, va_list Args);
	void ShowMessageBox(EMsgType Type, const char* pMsg);

public:

	CLogger(): _IsOpen(false), pLogFile(NULL) { __ConstructSingleton; }
	virtual ~CLogger() { if (IsOpen()) Close(); __DestructSingleton; }

	bool Open(const char* pAppName, const CString& FilePath = CString::Empty);
	void Close();
	bool IsOpen() const { return _IsOpen; }

	void Print(const char* pMsg, va_list Args);
	void Message(const char* pMsg, va_list Args);
	void Error(const char* pMsg, va_list Args);
	void OutputDebug(const char* pMsg, va_list Args);

	CLineBuffer* GetLineBuffer() { return &LineBuffer; }
};

}

#endif
