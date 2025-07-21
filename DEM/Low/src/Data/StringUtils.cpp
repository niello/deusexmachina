#include "StringUtils.h"

namespace StringUtils
{

char* LastOccurrenceOf(const char* pStrStart, const char* pStrEnd, const char* pSubStr, UPTR SubStrLen)
{
	if (!pStrStart || !pSubStr || !*pStrStart || !*pSubStr) return nullptr;

	if (!pStrEnd) pStrEnd = pStrStart + strlen(pStrStart);
	else if (pStrEnd <= pStrStart) return nullptr;

	UPTR SubStrLastIdx = (SubStrLen ? SubStrLen : strlen(pSubStr)) - 1;

	const char LastChar = pSubStr[SubStrLastIdx];
	const char* pCurr = pStrEnd;
	const char* pStop = pStrStart + SubStrLastIdx;
	while (pCurr >= pStop)
	{
		if (*pCurr == LastChar && memcmp(pCurr - SubStrLastIdx, pSubStr, SubStrLastIdx) == 0) return (char*)(pCurr - SubStrLastIdx);
		--pCurr;
	}

	return nullptr;
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

bool ToBool(const char* pStr)
{
	constexpr char* True[] = { "true", "1", "yes", "on" };
	for (auto* pBool : True)
		if (!n_stricmp(pBool, pStr)) return true;

	constexpr char* False[] = { "false", "0", "no", "off" };
	for (auto* pBool : False)
		if (!n_stricmp(pBool, pStr)) return false;

	Sys::Error("Invalid string value for bool!\n");
	return false;
}
//---------------------------------------------------------------------

//!!!non-optimal, can rewrite in a reverse order to minimize memmove sizes!
// Adds space for each multiline comment stripped to preserve token delimiting in a "name1/*comment*/name2" case
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
				*pFound = ' ';
				++pFound;
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
