#pragma once
#ifndef __DEM_L1_STRING_TOKENIZER_H__
#define __DEM_L1_STRING_TOKENIZER_H__

#include <StdDEM.h>

// Helper class for splitting read-only strings to tokens with minimal overhead

namespace Data
{

class CStringTokenizer
{
protected:

	const char*	pString;
	const char*	pCursor;
	char*		pBuffer;
	UPTR		BufferSize;
	char		CurrDelimiter;
	bool		FreeBuffer;

public:

	CStringTokenizer(const char* String, char* Buffer = NULL, UPTR BufSize = 0);
	~CStringTokenizer() { if (FreeBuffer) n_free(pBuffer); }

	void		ResetCursor() { pCursor = pString; }
	const char*	GetCurrToken() const { return pBuffer; }
	char		GetCurrDelimiter() const { return CurrDelimiter; }
	const char*	GetNextToken(const char* pSplitChars);
	const char*	GetNextToken(char SplitChar);
	const char*	GetNextToken(const char* pSplitChars, char Fence);
	bool		IsLast() const { return !pCursor || !*pCursor; }
};

inline CStringTokenizer::CStringTokenizer(const char* String, char* Buffer, UPTR BufSize):
	pString(String),
	pCursor(String),
	pBuffer(Buffer),
	BufferSize(BufSize),
	CurrDelimiter(0)
{
	if (pBuffer)
	{
		n_assert_dbg(BufferSize);
		FreeBuffer = false;
	}
	else
	{
		BufferSize = strlen(String) + 1;
		pBuffer = (char*)n_malloc(BufferSize);
		FreeBuffer = true;
	}

	pBuffer[0] = 0;
}
//---------------------------------------------------------------------

inline const char* CStringTokenizer::GetNextToken(const char* pSplitChars)
{
	if (!pCursor) return NULL;

	const char* pTokenEnd = strpbrk(pCursor, pSplitChars);
	if (pTokenEnd)
	{
		UPTR TokenSize = pTokenEnd - pCursor;
		n_assert2_dbg(TokenSize < BufferSize, "CStringTokenizer > Buffer overflow");
		memcpy(pBuffer, pCursor, TokenSize);
		pBuffer[TokenSize] = 0;
		CurrDelimiter = *pTokenEnd;
		++pTokenEnd;
	}
	else
	{
		n_verify_dbg(strcpy_s(pBuffer, BufferSize, pCursor) == 0);
		CurrDelimiter = 0;
	}

	pCursor = pTokenEnd;
	return pBuffer;
}
//---------------------------------------------------------------------

inline const char* CStringTokenizer::GetNextToken(char SplitChar)
{
	if (!pCursor) return NULL;

	const char* pTokenEnd = strchr(pCursor, SplitChar);
	if (pTokenEnd)
	{
		UPTR TokenSize = pTokenEnd - pCursor;
		n_assert2_dbg(TokenSize < BufferSize, "CStringTokenizer > Buffer overflow");
		memcpy(pBuffer, pCursor, TokenSize);
		pBuffer[TokenSize] = 0;
		CurrDelimiter = *pTokenEnd;
		++pTokenEnd;
	}
	else
	{
		n_verify_dbg(strcpy_s(pBuffer, BufferSize, pCursor) == 0);
		CurrDelimiter = 0;
	}

	pCursor = pTokenEnd;
	return pBuffer;
}
//---------------------------------------------------------------------

inline const char* CStringTokenizer::GetNextToken(const char* pSplitChars, char Fence)
{
	if (!pCursor) return NULL;

	char* pBuf = pBuffer;

	char Chr = *pCursor;
	if (Chr)
	{
		const char* pTokenEnd;
		if (Chr == Fence && (pTokenEnd = strchr(pCursor + 1, Fence)))
		{
			++pCursor;
			UPTR TokenSize = pTokenEnd - pCursor;
			n_assert2_dbg(TokenSize < BufferSize, "CStringTokenizer > Buffer overflow");
			memcpy(pBuffer, pCursor, TokenSize);
			pBuffer[TokenSize] = 0;
			pCursor = pTokenEnd;
			CurrDelimiter = *pTokenEnd;
			return pBuffer;
		}
		else return GetNextToken(pSplitChars);
	}
	else
	{
		pCursor = NULL;
		CurrDelimiter = 0;
		return NULL;
	}
}
//---------------------------------------------------------------------

}

#endif
