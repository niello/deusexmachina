#pragma once
#include <StdDEM.h>

// System functions and macros

namespace Sys
{
	enum EMsgType
	{
		MsgType_Log,		// Silent log message
		MsgType_Message,	// Important message to the user, possibly in a form of message box
		MsgType_DbgOut,		// Message to debug output window
		MsgType_Error		// Error message
	};

	enum EMsgBoxButton
	{
		MBB_OK		= 0x01,
		MBB_Cancel	= 0x02,
		MBB_Abort	= 0x04,
		MBB_Retry	= 0x08,
		MBB_Ignore	= 0x10,
		MBB_Yes		= 0x20,
		MBB_No		= 0x40
	};

	typedef void (*FLogHandler)(EMsgType Type, std::string_view Message);

	// Assertions and program termination
	void			DebugBreak();
	void			Crash(const char* pFile, int Line, std::string_view Message);
	bool			TraceStack(char* pTrace, unsigned int MaxLength);
	bool			ReportAssertionFailure(const char* pExpression, std::string_view Message, const char* pFile, int Line, const char* pFunc = nullptr);
	void            Error(std::string_view Message);
	//!!!need non-terminating error!

	// Logging
	void            Log(std::string_view Message);
	void            DbgOut(std::string_view Message);
	void            Message(std::string_view Message);

	// System UI
	EMsgBoxButton	ShowMessageBox(EMsgType Type, std::string_view HeaderText, std::string_view Message, unsigned int Buttons = MBB_OK);

	// Threading
	void			Sleep(unsigned long MSec); //!!!???to Thread namespace/class?!
	void            SetCurrentThreadName(std::string_view Name);
	void            SetCurrentThreadAffinity(size_t CPUIndex);
	void            SetCurrentThreadAffinity(std::initializer_list<size_t> CPUIndices);

	// Input
	bool			GetKeyName(U8 ScanCode, bool ExtendedKey, std::string& OutName);

	// Timing
	double			GetAppTime();
}

// See http://cnicholson.net/2009/02/stupid-c-tricks-adventures-in-assert/
#ifdef DEM_NO_ASSERT
	#define n_verify(exp)			do { (exp); } while(0)
	#define n_assert(exp)			do { (void)sizeof(exp); } while(0)
	#define n_assert2(exp, msg)		do { (void)sizeof(exp); } while(0)
#else
	#define n_verify(exp)			do { if (!(exp)) if (::Sys::ReportAssertionFailure(#exp, nullptr, __FILE__, __LINE__, __FUNCTION__)) __debugbreak(); } while(0)
	#define n_assert(exp)			do { if (!(exp)) if (::Sys::ReportAssertionFailure(#exp, nullptr, __FILE__, __LINE__, __FUNCTION__)) __debugbreak(); } while(0)
	#define n_assert2(exp, msg)		do { if (!(exp)) if (::Sys::ReportAssertionFailure(#exp, msg, __FILE__, __LINE__, __FUNCTION__)) __debugbreak(); } while(0)
#endif

#ifdef _DEBUG
	#define n_verify_dbg(exp)		n_verify(exp)
	#define n_assert_dbg(exp)		n_assert(exp)
	#define n_assert2_dbg(exp, msg)	n_assert2(exp, msg)
	#define DBG_ONLY(call)			call
#else
	#define n_verify_dbg(exp)		do { (exp); } while(0)
	#define n_assert_dbg(exp)		do { (void)sizeof(exp); } while(0)
	#define n_assert2_dbg(exp, msg)	do { (void)sizeof(exp); } while(0)
	#define DBG_ONLY(call)
#endif

#define NOT_IMPLEMENTED				do { ::Sys::Error(DEM_FUNCTION_NAME ## " > IMPLEMENT ME!!!\n"); } while(0)
#define NOT_IMPLEMENTED_MSG(msg)	do { ::Sys::Error(DEM_FUNCTION_NAME ## " > IMPLEMENT ME!!!\n" ## msg ## "\n"); } while(0)
