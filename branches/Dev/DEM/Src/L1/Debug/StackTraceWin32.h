#ifndef N_STACKTRACE_H
#define N_STACKTRACE_H

#include <kernel/ntypes.h>
#include <windows.h>
#include <dbghelp.h>

// Win32 stack trace utility

namespace Debug
{

class CStackTraceWin32
{
protected:

	struct CParams
	{
		CStackTraceWin32*	pSelf;
		DWORD64				Base;
	};

	static char	CharBuffer[1 << 15];
	HANDLE		hProcess;

	static BOOL CALLBACK EnumSymbolsCallback(PSYMBOL_INFO symbol, ULONG symbolSize, PVOID userContext);

	void			WalkStack(HANDLE thread, CONTEXT& context);
	void			ShowFrame(STACKFRAME64& frame);
	void			ShowLocals(STACKFRAME64& frame);
	void			ResetCharBuffer() { CharBuffer[0] = 0; }
	void __cdecl	Print(const char* msg, ...);

public:

	CStackTraceWin32(): hProcess(GetCurrentProcess()) {}

	const char* TraceStack();
};

}

#endif
