//------------------------------------------------------------------------------
//  nwin32stacktrace.cc
//
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "kernel/nwin32stacktrace.h"
#include "util/nstring.h"

char nWin32StackTrace::CharBuffer[1<<15] = { 0 };

//------------------------------------------------------------------------------
/*
*/
nWin32StackTrace::nWin32StackTrace()
{
    this->process = GetCurrentProcess();
}

//------------------------------------------------------------------------------
/*
*/
nWin32StackTrace::~nWin32StackTrace()
{
    // empty
}

//------------------------------------------------------------------------------
/*
*/
void
nWin32StackTrace::ResetCharBuffer()
{
    // reset the character buffer
    CharBuffer[0] = 0;
}

//------------------------------------------------------------------------------
/*
*/
void
nWin32StackTrace::Print(const char* str, ...)
{
    va_list argList;
    va_start(argList, str);
    char tmpLine[1024];
    _vsnprintf(tmpLine, sizeof(tmpLine), str, argList);
    va_end(argList);

    // append
    int len = strlen(CharBuffer) + strlen(tmpLine);
    if (len < (sizeof(CharBuffer) - 1))
    {
        strcat(CharBuffer, tmpLine);
    }
}

//------------------------------------------------------------------------------
/*
*/
const char*
nWin32StackTrace::TraceStack()
{
    HANDLE thread = GetCurrentThread();
    CONTEXT context;
    ZeroMemory(&context, sizeof(context));
    context.ContextFlags = CONTEXT_FULL;
    GetThreadContext(thread, &context);
    this->WalkStack(thread, context);
    return CharBuffer;
}

//------------------------------------------------------------------------------
/*
*/
void
nWin32StackTrace::WalkStack(HANDLE thread, CONTEXT& context)
{
    // add executable's directory to the search path
    char buf[N_MAXPATH];
    DWORD strLen = GetModuleFileName(0,  buf, sizeof(buf));
    nString path = buf;
    nString dirPath = path.ExtractDirName();
    SymInitialize(this->process, (PSTR)dirPath.Get(), true);

    STACKFRAME64 stackFrame = { 0 };
    stackFrame.AddrPC.Offset    = context.Eip;
    stackFrame.AddrPC.Mode      = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context.Ebp;
    stackFrame.AddrFrame.Mode   = AddrModeFlat;

    // reset character buffer
    this->ResetCharBuffer();

    // dump stack frames
    int frameNum;
    for (frameNum = 0;
        StackWalk64(
            IMAGE_FILE_MACHINE_I386,
            this->process,
            thread,
            &stackFrame,
            &context,
            0 /* ReadMemoryRoutine */,
            SymFunctionTableAccess64,
            SymGetModuleBase64,
            NULL /* TranslateAddress */);
        frameNum++)
    {
        this->ShowFrame(stackFrame);
    }

    SymCleanup(this->process);
}

//------------------------------------------------------------------------------
/*
*/
void
nWin32StackTrace::ShowFrame(STACKFRAME64& frame)
{
    const int maxNameLen = 512;
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)n_malloc(sizeof(SYMBOL_INFO) + maxNameLen);
    ZeroMemory(symbol, sizeof(SYMBOL_INFO) + maxNameLen);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = maxNameLen;

    IMAGEHLP_LINE64 line;
    ZeroMemory(&line, sizeof(line));
    line.SizeOfStruct = sizeof(line);

    if (frame.AddrPC.Offset == 0)
    {
        this->Print("Error: EIP=0\n");
        free(symbol);
        return;
    }

    DWORD64 offset;
    if (SymFromAddr(this->process, frame.AddrPC.Offset, &offset, symbol))
    {
        // this->Print("%-20s\t", symbol->Name);
        this->Print("%s(", symbol->Name);
        this->ShowLocals(frame);
        this->Print(")\n");
    }
    else
    {
        this->Print("(no symbol name)\n");
    }

    /*
    DWORD displacement;
    if (SymGetLineFromAddr64(this->process, frame.AddrPC.Offset, &displacement, &line))
    {
        this->Print("<%s@%d>\n", line.FileName, line.LineNumber);
    }
    else
    {
        this->Print("(no file name)\n");
    }
    */
    // this->ShowLocals(frame);
    n_free(symbol);
}

//------------------------------------------------------------------------------
/*
*/
void
nWin32StackTrace::ShowLocals(STACKFRAME64& frame)
{
    IMAGEHLP_STACK_FRAME sf;
    ZeroMemory(&sf, sizeof(sf));
    sf.BackingStoreOffset   = frame.AddrBStore.Offset;
    sf.FrameOffset          = frame.AddrFrame.Offset;
    sf.FuncTableEntry       = (ULONG64)frame.FuncTableEntry;
    sf.InstructionOffset    = frame.AddrPC.Offset;
    sf.Params[0]            = frame.Params[0];
    sf.Params[1]            = frame.Params[1];
    sf.Params[2]            = frame.Params[2];
    sf.Params[3]            = frame.Params[3];
    sf.ReturnOffset         = frame.AddrReturn.Offset;
    sf.StackOffset          = frame.AddrStack.Offset;
    sf.Virtual              = frame.Virtual;

    if (SymSetContext(this->process, &sf, 0))
    {
        Params params = {this, frame.AddrStack.Offset};
        if (!SymEnumSymbols(
            this->process,
            0, /* Base of Dll */
            "[a-zA-Z0-9_]*", /* mask */
            this->EnumSymbolsCallback,
            &params /* User context */
            ))
        {
            this->Print("no symbols available");
        }
    }
}

//------------------------------------------------------------------------------
/*
*/
BOOL CALLBACK
nWin32StackTrace::EnumSymbolsCallback(PSYMBOL_INFO symbol, ULONG symbolSize, PVOID userContext)
{
    Params* params = (Params*)userContext;
    DWORD64 addr = params->base + symbol->Address - 8;
    DWORD   data = *((DWORD*)addr);

    if (symbol->Flags & IMAGEHLP_SYMBOL_INFO_PARAMETER)
    {
        params->self->Print("%s=%d ", symbol->Name, data);
    }

/*
    params->self->Print("\t%c %-20s = %08x\n",
        (symbol->Flags & IMAGEHLP_SYMBOL_INFO_PARAMETER) ? 'P' : ' ',
        symbol->Name,
        data);
*/

    return true;
}
