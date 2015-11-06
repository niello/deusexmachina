#include "StringUtils.h"

namespace StringUtils
{

char* LastOccurrenceOf(const char* pStrStart, const char* pStrEnd, const char* pSubStr, DWORD SubStrLen)
{
	if (!pStrStart || !pSubStr || !*pStrStart || !*pSubStr) return NULL;

	if (!pStrEnd) pStrEnd = pStrStart + strlen(pStrStart);
	else if (pStrEnd <= pStrStart) return NULL;

	DWORD SubStrLastIdx = (SubStrLen ? SubStrLen : strlen(pSubStr)) - 1;

	const char LastChar = pSubStr[SubStrLastIdx];
	const char* pCurr = pStrEnd;
	const char* pStop = pStrStart + SubStrLastIdx;
	while (pCurr >= pStop)
	{
		if (*pCurr == LastChar && memcmp(pCurr - SubStrLastIdx, pSubStr, SubStrLastIdx) == 0) return (char*)(pCurr - SubStrLastIdx);
		--pCurr;
	}

	return NULL;
}
//---------------------------------------------------------------------

// A string matching function using Tcl's matching rules.
bool MatchesPattern(const char* pStr, const char* pPattern)
{
	char c2;

	while (true)
	{
		if (!*pPattern) return !*pStr;
		if (!*pStr && *pPattern != '*') return false;
		if (*pPattern == '*')
		{
			++pPattern;
			if (!*pPattern) return true;
			while (true)
			{
				if (MatchesPattern(pStr, pPattern)) return true;
				if (!*pStr) return false;
				++pStr;
			}
		}
		if (*pPattern == '?') goto match;
		if (*pPattern == '[')
		{
			++pPattern;
			while (true)
			{
				if (*pPattern == ']' || !*pPattern) return false;
				if (*pPattern == *pStr) break;
				if (pPattern[1] == '-')
				{
					c2 = pPattern[2];
					if (!c2) return false;
					if (*pPattern <= *pStr && c2 >= *pStr) break;
					if (*pPattern >= *pStr && c2 <= *pStr) break;
					pPattern += 2;
				}
				++pPattern;
			}
			while (*pPattern != ']')
			{
				if (!*pPattern)
				{
					--pPattern;
					break;
				}
				++pPattern;
			}
			goto match;
		}

		if (*pPattern == '\\')
		{
			++pPattern;
			if (!*pPattern) return false;
		}
		if (*pPattern != *pStr) return false;

match:
		++pPattern;
		++pStr;
	}
}
//---------------------------------------------------------------------

CString FromMatrix44(const matrix44& m)
{
	CString Str;
	Str.Format(	"%.6f, %.6f, %.6f, %.6f, "
				"%.6f, %.6f, %.6f, %.6f, "
				"%.6f, %.6f, %.6f, %.6f, "
				"%.6f, %.6f, %.6f, %.6f",
				m.M11, m.M12, m.M13, m.M14,
				m.M21, m.M22, m.M23, m.M24,
				m.M31, m.M32, m.M33, m.M34,
				m.M41, m.M42, m.M43, m.M44);
	return Str;
}
//---------------------------------------------------------------------

bool ToBool(const char* pStr)
{
	static const char* Bools[] = { "no", "yes", "off", "on", "false", "true", NULL };
	int i = 0;
	while (Bools[i])
	{
		if (!n_stricmp(Bools[i], pStr)) return (i & 1);
		++i;
	}
	Sys::Error("Invalid string value for bool!\n");
	FAIL;
}
//---------------------------------------------------------------------

//!!!non-optimal, can rewrite in a reverse order to minimize memmove sizes!
UPTR StripComments(char* pStr, const char* pSingleLineComment, const char* pMultiLineCommentStart, const char* pMultiLineCommentEnd)
{
	UPTR Len = strlen(pStr);

	if (pMultiLineCommentStart && pMultiLineCommentEnd)
	{
		UPTR MLCSLen = strlen(pMultiLineCommentStart);
		UPTR MLCELen = strlen(pMultiLineCommentEnd);
		char* pFound;
		while (pFound = strstr(pStr, pMultiLineCommentStart))
		{
			char* pEnd = strstr(pFound + MLCSLen, pMultiLineCommentEnd);
			if (pEnd)
			{
				const char* pFirstValid = pEnd + MLCELen;
				memmove(pFound, pFirstValid, Len - (pFirstValid - pStr));
				Len -= (pFirstValid - pFound);
				pStr[Len] = 0;
			}
			else
			{
				*pFound = 0;
				Len = pFound - pStr;
			}
		}
	}

	if (pSingleLineComment)
	{
		UPTR SLCLen = strlen(pSingleLineComment);
		char* pFound;
		while (pFound = strstr(pStr, pSingleLineComment))
		{
			char* pEnd = strpbrk(pFound + SLCLen, "\n\r");
			if (pEnd)
			{
				const char* pFirstValid = pEnd + 1;
				memmove(pFound, pFirstValid, Len - (pFirstValid - pStr));
				Len -= (pFirstValid - pFound);
				pStr[Len] = 0;
			}
			else
			{
				*pFound = 0;
				Len = pFound - pStr;
			}
		}
	}

	return Len;
}
//---------------------------------------------------------------------

}
