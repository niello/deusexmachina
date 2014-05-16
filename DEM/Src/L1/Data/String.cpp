#include "String.h"

const CString CString::Empty;

//!!!to CString/Utils!
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

//!!!to CString/Utils!
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

//!!!Write preallocation and growing as in CArray!
void CString::Reserve(DWORD NewLength)
{
	Sys::Error("NOT IMPLEMENTED!");
	//Clear();
	//if (NewLength >= LOCAL_STRING_SIZE)
	//{
	//	pString = (char*)n_malloc(NewLength + 1);
	//	memset(pString, 0, NewLength + 1);
	//	Len = NewLength;
	//}
	//else
	//{
	//	memset(pLocalString, 0, NewLength + 1);
	//	LocalLen = (ushort)NewLength;
	//}
}
//---------------------------------------------------------------------

void CString::Set(const char* pSrc, int SrcLength)
{
	Clear();
	if (!pSrc) return;
	if (SrcLength >= LOCAL_STRING_SIZE)
	{
		pString = (char*)n_malloc(SrcLength + 1);
		memcpy(pString, pSrc, SrcLength);
		pString[SrcLength] = 0;
		Len = SrcLength;
	}
	else
	{
		memcpy(pLocalString, pSrc, SrcLength);
		pLocalString[SrcLength] = 0;
		LocalLen = (ushort)SrcLength;
	}
}
//---------------------------------------------------------------------

int CString::FindStringIndex(const CString& Str, DWORD StartIdx) const
{
	DWORD StrLen = Length();
	n_assert(Str.IsValid() && StartIdx >= 0 && StartIdx < StrLen);
	const char* pStr = CStr();
	const char* pOtherStr = Str.CStr();
	DWORD OtherLen = Str.Length();
	for (DWORD i = StartIdx; i < StrLen; ++i)
	{
		if (StrLen - i < OtherLen) break;
		if (!strncmp(pStr + i, pOtherStr, OtherLen)) return i;
	}
	return INVALID_INDEX;
}
//---------------------------------------------------------------------

void CString::AppendRange(const char* str, uint CharCount)
{
    n_assert(str);
    if (CharCount > 0)
    {
        uint rlen = CharCount;
        uint tlen = Length() + rlen;
        if (pString)
        {
            char* ptr = (char*)n_malloc(tlen + 1);
            strcpy_s(ptr, tlen + 1, pString);
            strncat_s(ptr, tlen + 1, str, CharCount);
            n_free((void*)pString);
            pString = ptr;
            Len = tlen;
        }
        else if (pLocalString[0])
        {
            if (tlen >= LOCAL_STRING_SIZE)
            {
                char* ptr = (char*)n_malloc(tlen + 1);
                strcpy_s(ptr, tlen + 1, pLocalString);
                strncat_s(ptr, tlen + 1, str, CharCount);
                pLocalString[0] = 0;
                pString = ptr;
                Len = tlen;
            }
            else
            {
                strncat_s(pLocalString, str, CharCount);
                LocalLen = (ushort)tlen;
            }
        }
        else Set(str, CharCount);
    }
}
//---------------------------------------------------------------------

CString CString::Replace(const char* pMatch, const char* pReplaceWith) const
{
	n_assert(pMatch && pReplaceWith);

	const char* pStr = CStr();
	int MatchLen = strlen(pMatch);
	CString Result;

	const char* pFound;
	while (pFound = strstr(pStr, pMatch))
	{
		Result.AppendRange(pStr, pFound - pStr);
		Result.Add(pReplaceWith);
		pStr = pFound + MatchLen;
	}
	Result.Add(pStr);
	return Result;
}
//---------------------------------------------------------------------

void CString::TrimInplace(const char* CharSet, bool Left, bool Right)
{
	n_assert(CharSet);
	if (IsEmpty() || !(Left || Right)) return;

	const char* pStrFirst = CStr();
	const char* pStrLast = pStrFirst + Length();
	const char* pStrStart = pStrFirst;
	char* pStrEnd = (char*)pStrLast;

	if (Left)
		while (pStrStart < pStrLast && strchr(CharSet, *pStrStart)) ++pStrStart;

	if (Right)
	{
		while (pStrEnd >= pStrFirst && strchr(CharSet, *pStrEnd)) --pStrEnd;
		if (pStrEnd < pStrLast) ++pStrEnd;
	}

	if (pStrStart == pStrFirst)
	{
		if (pStrEnd < pStrLast)
		{
			*pStrEnd = 0;
			SetLength(pStrEnd - pStrStart);
		}
	}
	else if (pStrEnd > pStrStart)
	{
		//!!!use memmove!
		DWORD NewLen = pStrEnd - pStrStart;
		char* pBuffer = (char*)_malloca(NewLen);
		memcpy_s(pBuffer, NewLen, pStrStart, NewLen);
		Set(pBuffer, NewLen);
		_freea(pBuffer);
	}
	else Clear();
}
//---------------------------------------------------------------------
