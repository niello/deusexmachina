#include "String.h"

#include <Data/StringUtils.h>
#include <stdio.h>

const CString CString::Empty;

void CString::Reallocate(UPTR NewMaxLength)
{
	if (NewMaxLength == MaxLength) return;

	if (NewMaxLength)
	{
		char* pNewString = (char*)std::realloc(pString, NewMaxLength + 1);
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

void CString::FormatWithArgs(const char* pFormatStr, va_list Args)
{
	va_list ArgList;
	va_copy(ArgList, Args);
	UPTR ReqLength = _vscprintf(pFormatStr, ArgList);
	va_end(ArgList);

	UPTR BufferSize = ReqLength + 1;
	if (ReqLength > MaxLength)
		pString = (char*)std::realloc(pString, BufferSize);
	_vsnprintf_s(pString, BufferSize, BufferSize, pFormatStr, Args);
	MaxLength = ReqLength;
	Length = ReqLength;
}
//---------------------------------------------------------------------

void CString::Trim(const char* pCharSet, bool Left, bool Right)
{
	if (!pCharSet || IsEmpty() || !(Left || Right)) return;

	const char* pStrStart = pString;
	char* pStrEnd = pString + Length;
	const char* pStrLast = pStrEnd;

	if (Left)
		while (pStrStart < pStrLast && strchr(pCharSet, *pStrStart)) ++pStrStart;

	if (Right)
	{
		while (pStrEnd >= pString && strchr(pCharSet, *pStrEnd)) --pStrEnd;
		if (pStrEnd < pStrLast) ++pStrEnd;
	}

	if (pStrStart == pString)
	{
		if (pStrEnd < pStrLast)
		{
			*pStrEnd = 0;
			Length = pStrEnd - pStrStart;
		}
	}
	else if (pStrEnd > pStrStart)
	{
		UPTR NewLen = pStrEnd - pStrStart;
		memmove(pString, pStrStart, NewLen);
		pString[NewLen] = 0;
		Length = NewLen;
	}
	else Clear();
}
//---------------------------------------------------------------------

void CString::Replace(const char* pSubStr, const char* pReplaceWith)
{
	if (!pSubStr || !pString || !*pSubStr || !*pString) return;

	char* pCurr = strstr(pString, pSubStr);
	if (!pCurr) return;

	UPTR SrcLen = strlen(pSubStr);
	UPTR ReplaceLen = pReplaceWith ? strlen(pReplaceWith) : 0;
	char* pEnd = pString + Length;

	if (ReplaceLen < SrcLen)
	{
		char* pDest = pCurr;
		char* pPrev;

		UPTR NewLength = Length;
		UPTR LengthDiff = SrcLen - ReplaceLen;
		do
		{
			if (ReplaceLen)
			{
				memmove(pDest, pReplaceWith, ReplaceLen);
				pDest += ReplaceLen;
			}
			NewLength -= LengthDiff;
			pPrev = pCurr + SrcLen;
			pCurr = strstr(pPrev, pSubStr);
			if (!pCurr) pCurr = pEnd;
			if (pPrev < pCurr)
			{
				UPTR MidLen = pCurr - pPrev;
				memmove(pDest, pPrev, MidLen);
				pDest += MidLen;
			}
		}
		while (pCurr < pEnd);

		pString[NewLength] = 0;
		Length = NewLength;
	}
	else if (ReplaceLen == SrcLen)
	{
		do
		{
			memmove(pCurr, pReplaceWith, ReplaceLen);
			pCurr = strstr(pCurr + SrcLen, pSubStr);
		}
		while (pCurr);
	}
	else
	{
		// We may need to reallocate string. Calc the room required.
		UPTR ReplaceCount = 0;
		do
		{
			++ReplaceCount;
			pCurr = strstr(pCurr + SrcLen, pSubStr);
		}
		while (pCurr);

		UPTR NewLength = Length + ReplaceCount * (ReplaceLen - SrcLen);
		char* pNewString = (NewLength > MaxLength) ? (char*)std::malloc(NewLength + 1) : pString;

		pCurr = pEnd;
		char* pDestEnd = pNewString + NewLength;
		char* pPrev;
		do
		{
			pPrev = pCurr;
			pCurr = StringUtils::LastOccurrenceOf(pString, pCurr, pSubStr, SrcLen);
			if (!pCurr) break;

			const char* pIntactStart = pCurr + SrcLen;
			if (pIntactStart < pPrev)
			{
				UPTR IntactLen = pPrev - pIntactStart;
				pDestEnd -= IntactLen;
				memmove(pDestEnd, pIntactStart, IntactLen);
			}

			pDestEnd -= ReplaceLen;
			memmove(pDestEnd, pReplaceWith, ReplaceLen);
		}
		while (true);

		if (pNewString != pString)
		{
			if (pString < pPrev) memmove(pNewString, pString, pPrev - pString);
			std::free(pString);
			pString = pNewString;
			MaxLength = NewLength;
		}

		pString[NewLength] = 0;
		Length = NewLength;
	}
}
//---------------------------------------------------------------------
