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
	LPCSTR	pSplitChars;
	LPCSTR	pCursor;
	LPSTR	pBuffer;		// Can write static variant with array of predefined size (inherit from this? or allow set ext buffer?)
	DWORD	BufferSize;
	bool	FreeBuffer;

	bool	IsCharInList(char Chr);

public:

	CStringTokenizer(LPCSTR String, LPCSTR SplitChars, LPSTR Buffer = NULL, DWORD BufSize = 0);
	~CStringTokenizer() { if (FreeBuffer) n_free(pBuffer); }

	void	ResetCursor() { pCursor = pString; }
	LPCSTR	GetCurrToken() const { return pBuffer; }
	LPCSTR	GetNextToken();
	LPCSTR	GetNextTokenSingleChar();
	bool	IsLast() const { return !pCursor || !*pCursor; }
};

inline CStringTokenizer::CStringTokenizer(LPCSTR String, LPCSTR SplitChars, LPSTR Buffer, DWORD BufSize):
	pString(String),
	pSplitChars(SplitChars),
	pCursor(String),
	pBuffer(Buffer),
	BufferSize(BufSize),
	FreeBuffer(false)
{
	n_assert_dbg(pSplitChars);
	n_assert_dbg(!pBuffer || BufferSize);

	if (!pBuffer)
	{
		BufferSize = 0;
		DWORD CurrSize = 0;
		if (pCursor)
		{
			while (*pCursor)
			{
				char Chr = *pCursor;
				if (IsCharInList(Chr))
				{
					if (CurrSize > BufferSize) BufferSize = CurrSize;
					CurrSize = 0;
				}
				else ++CurrSize;
				++pCursor;
			}
		}
		if (CurrSize > BufferSize) BufferSize = CurrSize;

		++BufferSize;
		pBuffer = (LPSTR)n_malloc(BufferSize);
		FreeBuffer = true;

		ResetCursor();
	}
}
//---------------------------------------------------------------------

inline LPCSTR CStringTokenizer::GetNextToken()
{
	if (!pCursor) return NULL;

	LPSTR pBuf = pBuffer;

	while (*pCursor)
	{
		n_assert_dbg(pBuf < pBuffer + BufferSize);

		char Chr = *pCursor;
		++pCursor;
		if (IsCharInList(Chr))
		{
			*pBuf = 0;
			return pBuffer;
		}
		else *pBuf++ = Chr;
	}

	n_assert_dbg(pBuf < pBuffer + BufferSize);

	pCursor = NULL;
	*pBuf = 0;
	return pBuffer;
}
//---------------------------------------------------------------------

// This method is written to avoid indirection when string is tokenized
// by a single character. Be careful using this with self-allocated buffer,
// because it calculates its length based on all characters of a split set.
inline LPCSTR CStringTokenizer::GetNextTokenSingleChar()
{
	if (!pCursor) return NULL;

	LPSTR pBuf = pBuffer;
	char SplitChr = *pSplitChars;

	while (*pCursor)
	{
		n_assert_dbg(pBuf < pBuffer + BufferSize);

		char Chr = *pCursor;
		++pCursor;
		if (Chr == SplitChr)
		{
			*pBuf = 0;
			return pBuffer;
		}
		else *pBuf++ = Chr;
	}

	n_assert_dbg(pBuf < pBuffer + BufferSize);

	pCursor = NULL;
	*pBuf = 0;
	return pBuffer;
}
//---------------------------------------------------------------------

inline bool CStringTokenizer::IsCharInList(char Chr)
{
	LPCSTR pSplitChar = pSplitChars;
	while (*pSplitChar)
		if (Chr == *pSplitChar++) OK;
	FAIL;
}
//---------------------------------------------------------------------

}

#endif
