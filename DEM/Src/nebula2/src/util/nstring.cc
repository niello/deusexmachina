#include "util/nstring.h"

const nString nString::Empty;

void nString::AppendRange(const char* str, uint CharCount)
{
    n_assert(str);
    if (CharCount > 0)
    {
        uint rlen = CharCount;
        uint tlen = Length() + rlen;
        if (pString)
        {
            char* ptr = (char*)n_malloc(tlen + 1);
            strcpy(ptr, pString);
            strncat(ptr, str, CharCount);
            n_free((void*)pString);
            pString = ptr;
            Len = tlen;
        }
        else if (pLocalString[0])
        {
            if (tlen >= LOCAL_STRING_SIZE)
            {
                char* ptr = (char*)n_malloc(tlen + 1);
                strcpy(ptr, pLocalString);
                strncat(ptr, str, CharCount);
                pLocalString[0] = 0;
                pString = ptr;
                Len = tlen;
            }
            else
            {
                strncat(pLocalString, str, CharCount);
                LocalLen = (ushort)tlen;
            }
        }
        else Set(str, CharCount);
    }
}
//---------------------------------------------------------------------

nString nString::Replace(const char* pMatch, const char* pReplaceWith) const
{
	n_assert(pMatch && pReplaceWith);

	const char* pStr = CStr();
	int MatchLen = strlen(pMatch);
	nString Result;

	const char* pFound;
	while ((pFound = strstr(pStr, pMatch)))
	{
		Result.AppendRange(pStr, pFound - pStr);
		Result.Add(pReplaceWith);
		pStr = pFound + MatchLen;
	}
	Result.Add(pStr);
	return Result;
}
//---------------------------------------------------------------------

// Returns a new string which is this string, stripped on the left side by all characters in the char set
nString nString::TrimLeft(const char* CharSet) const
{
    n_assert(CharSet);
    if (IsEmpty()) return *this;

    int charSetLen = strlen(CharSet);
    int thisIndex = 0;
    bool stopped = false;
    while (!stopped && (thisIndex < Length()))
    {
        int charSetIndex;
        bool match = false;
        for (charSetIndex = 0; charSetIndex < charSetLen; charSetIndex++)
        {
            if ((*this)[thisIndex] == CharSet[charSetIndex])
            {
                // a match
                match = true;
                break;
            }
        }
        if (!match) stopped = true;
        else ++thisIndex;
    }
    nString trimmedString(&(CStr()[thisIndex]));
    return trimmedString;
}
//---------------------------------------------------------------------

// Returns a new string, which is this string, stripped on the right side by all characters in the char set
nString nString::TrimRight(const char* CharSet) const
{
    n_assert(CharSet);
    if (IsEmpty()) return *this;

    int charSetLen = strlen(CharSet);
    int thisIndex = Length() - 1;
    bool stopped = false;
    while (!stopped && (thisIndex < Length()))
    {
        int charSetIndex;
        bool match = false;
        for (charSetIndex = 0; charSetIndex < charSetLen; charSetIndex++)
        {
            if ((*this)[thisIndex] == CharSet[charSetIndex])
            {
                // a match
                match = true;
                break;
            }
        }
        if (!match) stopped = true;
        else --thisIndex;
    }
    nString trimmedString;
    trimmedString.Set(CStr(), thisIndex + 1);
    return trimmedString;
}
//---------------------------------------------------------------------
