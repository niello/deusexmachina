#include <Core/Logger.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace Core
{

bool ReportAssertionFailure(const char* pExpression, const char* pMessage, const char* pFile, int Line, const char* pFunc)
{
	const char* pMsg = pMessage ? pMessage : "none";

	//!!!msgbox can have buttons to continue or fail!
	if (CoreLoggerExists)
	{
		//!!!refactor logger!
		//!!!log as in 'else', with equal offsets, but show in msg box as here!
		char Buf[8192];
		sprintf_s(Buf, sizeof(Buf) - 1, "*** DEM ASSERTION FAILED ***\nMessage: %s\nExpression: %s\nFile: %s\nLine: %d\nFunction: %s\n", pMsg, pExpression, pFile, Line, pFunc);
		CoreLogger->Error(Buf, NULL);
		OK; //!!!now always fails!
	}
	else
	{
		printf("*** DEM ASSERTION FAILED ***\nMessage:    %s\nExpression: %s\nFile:       %s\nLine:       %d\nFunction:   %s\n", pMsg, pExpression, pFile, Line, pFunc);
		fflush(stdout);
		OK;
	}
}
//---------------------------------------------------------------------

// Critical error, program will be closed
void __cdecl Error(const char* pMsg, ...)
{
	va_list ArgList;
	va_start(ArgList, pMsg);
	if (CoreLoggerExists) CoreLogger->Error(pMsg, ArgList);
	else
	{
		vprintf(pMsg, ArgList);
		fflush(stdout);
	}
	va_end(ArgList);
	abort();
}
//---------------------------------------------------------------------

}

// Message that will be shown to the user
void __cdecl n_message(const char* pMsg, ...)
{
	va_list ArgList;
	va_start(ArgList, pMsg);
	if (CoreLoggerExists) CoreLogger->Message(pMsg, ArgList);
	else vprintf(pMsg, ArgList);
	va_end(ArgList);
}
//---------------------------------------------------------------------

// Logable version of printf
void __cdecl n_printf(const char* pMsg, ...)
{
	va_list ArgList;
	va_start(ArgList,pMsg);
	if (CoreLoggerExists) CoreLogger->Print(pMsg, ArgList);
	else vprintf(pMsg, ArgList);
	va_end(ArgList);
}
//---------------------------------------------------------------------

// Message printed to the debug output window
void __cdecl n_dbgout(const char* pMsg, ...)
{
	va_list ArgList;
	va_start(ArgList,pMsg);
	if (CoreLoggerExists) CoreLogger->OutputDebug(pMsg, ArgList);
	else
	{
		vprintf(pMsg, ArgList);
		fflush(stdout);
	}
	va_end(ArgList);
}
//---------------------------------------------------------------------

void n_sleep(double sec)
{
	Sleep((int)(sec * 1000.0));
}
//---------------------------------------------------------------------

// A strdup() implementation using engine's malloc() override.
char* n_strdup(const char* from)
{
	n_assert(from);
	int BufLen = strlen(from) + 1;
	char* to = (char*)n_malloc(BufLen);
	if (to) strcpy_s(to, BufLen, from);
	return to;
}
//---------------------------------------------------------------------

// A string matching function using Tcl's matching rules.
bool n_strmatch(const char* str, const char* pat)
{
    char c2;

    while (true)
    {
        if (!*pat) return !*str;
        if (!*str && *pat != '*') return false;
        if (*pat == '*')
        {
            ++pat;
            if (!*pat) return true;
            while (true)
            {
                if (n_strmatch(str, pat)) return true;
                if (!*str) return false;
                ++str;
            }
        }
        if (*pat == '?') goto match;
        if (*pat == '[')
        {
            ++pat;
            while (true)
            {
                if (*pat == ']' || !*pat) return false;
                if (*pat == *str) break;
                if (pat[1] == '-')
                {
                    c2 = pat[2];
                    if (!c2) return false;
                    if (*pat <= *str && c2 >= *str) break;
                    if (*pat >= *str && c2 <= *str) break;
                    pat += 2;
                }
                ++pat;
            }
            while (*pat != ']')
            {
                if (!*pat)
                {
                    --pat;
                    break;
                }
                ++pat;
            }
            goto match;
        }

        if (*pat == '\\')
        {
            ++pat;
            if (!*pat) return false;
        }
        if (*pat != *str) return false;

match:
        ++pat;
        ++str;
    }
}
//---------------------------------------------------------------------
