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

	LPCSTR	pString;
	LPCSTR	pCursor;
	LPSTR	pBuffer;
	DWORD	BufferSize;
	bool	FreeBuffer;

public:

	CStringTokenizer(LPCSTR String, LPSTR Buffer = NULL, DWORD BufSize = 0);
	~CStringTokenizer() { if (FreeBuffer) n_free(pBuffer); }

	void	ResetCursor() { pCursor = pString; }
	LPCSTR	GetCurrToken() const { return pBuffer; }
	LPCSTR	GetNextToken(LPCSTR pSplitChars);
	LPCSTR	GetNextToken(char SplitChar);
	LPCSTR	GetNextToken(LPCSTR pSplitChars, char Fence);
	bool	IsLast() const { return !pCursor || !*pCursor; }
};

inline CStringTokenizer::CStringTokenizer(LPCSTR String, LPSTR Buffer, DWORD BufSize):
	pString(String),
	pCursor(String),
	pBuffer(Buffer),
	BufferSize(BufSize)
{
	if (pBuffer)
	{
		n_assert_dbg(BufferSize);
		FreeBuffer = false;
	}
	else
	{
		BufferSize = strlen(String) + 1;
		pBuffer = (LPSTR)n_malloc(BufferSize);
		FreeBuffer = true;
	}

	pBuffer[0] = 0;
}
//---------------------------------------------------------------------

inline LPCSTR CStringTokenizer::GetNextToken(LPCSTR pSplitChars)
{
	if (!pCursor) return NULL;

	const char* pTokenEnd = strpbrk(pCursor, pSplitChars);
	if (pTokenEnd)
	{
		DWORD TokenSize = pTokenEnd - pCursor;
		n_assert2_dbg(TokenSize < BufferSize, "CStringTokenizer > Buffer overflow");
		memcpy(pBuffer, pCursor, TokenSize);
		pBuffer[TokenSize] = 0;
	}
	else n_verify_dbg(strcpy_s(pBuffer, BufferSize, pCursor) == 0);

	pCursor = pTokenEnd;
	return pBuffer;
}
//---------------------------------------------------------------------

inline LPCSTR CStringTokenizer::GetNextToken(char SplitChar)
{
	if (!pCursor) return NULL;

	const char* pTokenEnd = strchr(pCursor, SplitChar);
	if (pTokenEnd)
	{
		DWORD TokenSize = pTokenEnd - pCursor;
		n_assert2_dbg(TokenSize < BufferSize, "CStringTokenizer > Buffer overflow");
		memcpy(pBuffer, pCursor, TokenSize);
		pBuffer[TokenSize] = 0;
	}
	else n_verify_dbg(strcpy_s(pBuffer, BufferSize, pCursor) == 0);

	pCursor = pTokenEnd;
	return pBuffer;
}
//---------------------------------------------------------------------

inline LPCSTR CStringTokenizer::GetNextToken(LPCSTR pSplitChars, char Fence)
{
	if (!pCursor) return NULL;

	LPSTR pBuf = pBuffer;

	char Chr = *pCursor;
	if (Chr)
	{
		const char* pTokenEnd;
		if (Chr == Fence && (pTokenEnd = strchr(pCursor + 1, Fence)))
		{
			++pCursor;
			DWORD TokenSize = pTokenEnd - pCursor;
			n_assert2_dbg(TokenSize < BufferSize, "CStringTokenizer > Buffer overflow");
			memcpy(pBuffer, pCursor, TokenSize);
			pBuffer[TokenSize] = 0;
			pCursor = pTokenEnd;
			return pBuffer;
		}
		else return GetNextToken(pSplitChars);
	}
	else
	{
		pCursor = NULL;
		return NULL;
	}
}
//---------------------------------------------------------------------

}

#endif
