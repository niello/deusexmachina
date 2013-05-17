#include "StackTraceWin32.h"
#include <util/nstring.h>

namespace Debug
{
char CStackTraceWin32::CharBuffer[1<<15] = { 0 };

void CStackTraceWin32::Print(const char* str, ...)
{
	va_list argList;
	va_start(argList, str);
	char tmpLine[1024];
	_vsnprintf(tmpLine, sizeof(tmpLine), str, argList);
	va_end(argList);

	// append
	int len = strlen(CharBuffer) + strlen(tmpLine);
	if (len < (sizeof(CharBuffer) - 1))
		strcat(CharBuffer, tmpLine);
}
//---------------------------------------------------------------------

const char* CStackTraceWin32::TraceStack()
{
	HANDLE thread = GetCurrentThread();
	CONTEXT context;
	ZeroMemory(&context, sizeof(context));
	context.ContextFlags = CONTEXT_FULL;
	GetThreadContext(thread, &context);
	WalkStack(thread, context);
	return CharBuffer;
}
//---------------------------------------------------------------------

void CStackTraceWin32::WalkStack(HANDLE thread, CONTEXT& context)
{
	// add executable's directory to the search path
	char buf[N_MAXPATH];
	DWORD strLen = GetModuleFileName(0,  buf, sizeof(buf));
	nString path = buf;
	nString dirPath = path.ExtractDirName();
	SymInitialize(hProcess, (PSTR)dirPath.CStr(), true);

	STACKFRAME64 stackFrame = { 0 };
	stackFrame.AddrPC.Offset    = context.Eip;
	stackFrame.AddrPC.Mode      = AddrModeFlat;
	stackFrame.AddrFrame.Offset = context.Ebp;
	stackFrame.AddrFrame.Mode   = AddrModeFlat;

	// reset character buffer
	ResetCharBuffer();

	// dump stack frames
	int frameNum;
	for (frameNum = 0;
		StackWalk64(
			IMAGE_FILE_MACHINE_I386,
			hProcess,
			thread,
			&stackFrame,
			&context,
			0 /* ReadMemoryRoutine */,
			SymFunctionTableAccess64,
			SymGetModuleBase64,
			NULL /* TranslateAddress */);
		frameNum++)
	{
		ShowFrame(stackFrame);
	}

	SymCleanup(hProcess);
}
//---------------------------------------------------------------------

void CStackTraceWin32::ShowFrame(STACKFRAME64& frame)
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
		Print("Error: EIP=0\n");
		free(symbol);
		return;
	}

	DWORD64 offset;
	if (SymFromAddr(hProcess, frame.AddrPC.Offset, &offset, symbol))
	{
		// Print("%-20s\t", symbol->Name);
		Print("%s(", symbol->Name);
		ShowLocals(frame);
		Print(")\n");
	}
	else Print("(no symbol name)\n");

	/*
	DWORD displacement;
	if (SymGetLineFromAddr64(hProcess, frame.AddrPC.Offset, &displacement, &line))
		Print("<%s@%d>\n", line.FileName, line.LineNumber);
	else
		Print("(no file name)\n");
	*/
	// ShowLocals(frame);
	n_free(symbol);
}
//---------------------------------------------------------------------

void CStackTraceWin32::ShowLocals(STACKFRAME64& frame)
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

	if (SymSetContext(hProcess, &sf, 0))
	{
		CParams params = {this, frame.AddrStack.Offset};
		if (!SymEnumSymbols(
				hProcess,
				0, /* Base of Dll */
				"[a-zA-Z0-9_]*", /* mask */
				EnumSymbolsCallback,
				&params /* User context */))
		{
			Print("no symbols available");
		}
	}
}
//---------------------------------------------------------------------

BOOL CALLBACK CStackTraceWin32::EnumSymbolsCallback(PSYMBOL_INFO symbol, ULONG symbolSize, PVOID userContext)
{
	CParams* params = (CParams*)userContext;
	DWORD64 addr = params->Base + symbol->Address - 8;
	DWORD data = *((DWORD*)addr);

	if (symbol->Flags & IMAGEHLP_SYMBOL_INFO_PARAMETER)
		params->pSelf->Print("%s=%d ", symbol->Name, data);

/*
	params->pSelf->Print("\t%c %-20s = %08x\n",
		(symbol->Flags & IMAGEHLP_SYMBOL_INFO_PARAMETER) ? 'P' : ' ',
		symbol->Name,
		data);
*/

	return true;
}
//---------------------------------------------------------------------

}