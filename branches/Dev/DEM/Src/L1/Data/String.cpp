#include "String.h"

#include <stdio.h>

const CString CString::Empty;

void CString::Reallocate(DWORD NewMaxLength)
{
	if (NewMaxLength == MaxLength) return;

	if (NewMaxLength)
	{
		char* pNewString = (char*)n_realloc(pString, NewMaxLength + 1);
		if (!pNewString)
		{
			Sys::Error("CString::Reallocate() > Allocation failed!\n");
			//Clear();
			return;
		}

		pString = pNewString;

		if (NewMaxLength < Length)
		{
			pString[NewMaxLength] = 0;
			Length = NewMaxLength;
		}

		MaxLength = NewMaxLength;
	}
	else Clear();
}
//---------------------------------------------------------------------

void CString::Trim(const char* CharSet, bool Left, bool Right)
{
	if (!CharSet || IsEmpty() || !(Left || Right)) return;

	const char* pStrFirst = CStr();
	const char* pStrLast = pStrFirst + Length;
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
			Length = pStrEnd - pStrStart;
		}
	}
	else if (pStrEnd > pStrStart)
	{
		DWORD NewLen = pStrEnd - pStrStart;
		char* pBuffer = (char*)_malloca(NewLen);
		memmove(pBuffer, pStrStart, NewLen);
		Set(pBuffer, NewLen);
		_freea(pBuffer);
	}
	else Clear();
}
//---------------------------------------------------------------------

void CString::FormatWithArgs(const char* pFormatStr, va_list Args)
{
	va_list ArgList;
	va_copy(ArgList, Args);
	DWORD ReqLength = _vscprintf(pFormatStr, ArgList);
	va_end(ArgList);

	if (ReqLength > MaxLength)
		pString = (char*)n_realloc(pString, ReqLength + 1);
	_vsnprintf_s(pString, ReqLength, ReqLength, pFormatStr, Args);
	pString[ReqLength] = 0;
	MaxLength = ReqLength;
	Length = ReqLength;
}
//---------------------------------------------------------------------

/*
int CString::FindStringIndex(const CString& Str, DWORD StartIdx) const
{
	DWORD StrLen = Length();
	n_assert(Str.IsValid() && StartIdx >= 0 && StartIdx < StrLen);
	const char* pStr = CStr();
	const char* pOtherStr = Str.CStr();
	DWORD OtherLen = Str.Length();
	for (DWORD i = StartIdx; i < StrLen; ++i)
	{
		if (StrLen - i < OtherLen) break; //!!!to the main condition!
		if (!strncmp(pStr + i, pOtherStr, OtherLen)) return i;
	}
	return INVALID_INDEX;
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
*/