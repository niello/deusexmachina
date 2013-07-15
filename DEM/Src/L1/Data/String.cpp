#include "String.h"

const CString CString::Empty;

//!!!Write preallocation and growing as in CArray!
void CString::Reserve(DWORD NewLength)
{
	n_error("NOT IMPLEMENTED!");
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
