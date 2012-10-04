#ifndef N_STACKTRACE_H
#define N_STACKTRACE_H
//------------------------------------------------------------------------------
/**
    @class nStackTrace
    @ingroup Kernel

    FIXME FIXME FIMXE

    This class implements experimental stacktracing under Win32, and
    hasn't been integrated so far with the rest of the code. Should be
    unified with Bruce's Linux stacktracer (can we use a similar approach
    like the nLogHandler class? One base class with platform specific
    subclasses? May require some work on the interface though.

    -Floh.

    (C) 2003 RadonLabs GmbH
*/
#include "kernel/ntypes.h"
#include <windows.h>
#include <dbghelp.h>

//------------------------------------------------------------------------------
class nWin32StackTrace
{
public:
    /// constructor
    nWin32StackTrace();
    /// destructor
    ~nWin32StackTrace();
    /// trace the stack
    const char* TraceStack();

protected:
    /// trace stack of given thread
    void WalkStack(HANDLE thread, CONTEXT& context);
    /// show single frame
    void ShowFrame(STACKFRAME64& frame);
    /// show local symbols of frame
    void ShowLocals(STACKFRAME64& frame);
    /// callback for local symbols
    static BOOL CALLBACK EnumSymbolsCallback(PSYMBOL_INFO symbol, ULONG symbolSize, PVOID userContext);
    /// reset the character buffer
    void ResetCharBuffer();
    /// append a message to the character buffer
    void __cdecl Print(const char* msg, ...);

    static char CharBuffer[1<<15];
    HANDLE process;
    struct Params
    {
        nWin32StackTrace* self;
        DWORD64 base;
    };
};
//------------------------------------------------------------------------------
#endif
