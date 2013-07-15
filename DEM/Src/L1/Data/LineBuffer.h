#pragma once
#ifndef __DEM_L1_LINE_BUFFER_H__
#define __DEM_L1_LINE_BUFFER_H__

#include <StdDEM.h>
#include <kernel/ntypes.h>

// A ring buffer for strings.
// (C) 2002 RadonLabs GmbH

class CLineBuffer
{
private:

	struct CLine
	{
		int		Len;
		int		ActualPos;
		char*	pString;

		CLine(): pString(NULL), Len(0), ActualPos(0) {}

		void		Set(char* pStr, int Length);
		void		Reset() { n_assert(pString); *pString = 0; ActualPos = 0; }
		const char*	Add(const char* pInStr);
	};

	enum
	{
		MAX_LINE_CHARS	= 80,
		MAX_LINES		= 256
	};

	char*	pBuffer;
	CLine	Lines[MAX_LINES];
	int		TailLine;
	int		HeadLine;

	int NextLine(int l) const { return (++l >= MAX_LINES) ? 0 : l; }
	int PrevLine(int l) const { return (--l < 0) ? MAX_LINES - 1 : l; }

public:

	CLineBuffer();
	~CLineBuffer() { if (pBuffer) n_free(pBuffer); }

	void		Put(const char* pString);
	const char*	GetLine(DWORD Idx) const { n_assert(Idx < MAX_LINES); return Lines[Idx].pString; }
	int			GetLines(const char** OutLines, int MaxCount) const;
	int			GetHeadLine() const { return HeadLine; }
	int			GetTailLine() const { return TailLine; }
	int			GetNextLine(int Idx) const { return (Idx == HeadLine) ? -1 : NextLine(Idx); }
	int			GetPrevLine(int Idx) const { return (Idx == TailLine) ? -1 : PrevLine(Idx); }
};

inline void CLineBuffer::CLine::Set(char* pStr, int Length)
{
	n_assert(pStr);
	pString = pStr;
	*pString = 0;
	Len = Length;
	ActualPos = 0;
}
//---------------------------------------------------------------------

// Appends string until pString is full, new pString or end of string is reached.
// If new pString, a pointer to the next char is returned, otherwise NULL.
// A '\r' in the string rewinds the cursor to the start of the pString. If the
// string buffer is full, a 0 is appended in any case. Newlines are not copied.
inline const char* CLineBuffer::CLine::Add(const char* pInStr)
{
	n_assert(pInStr);

	bool Running = true;
	do
	{
		char Chr = *pInStr++;
		if (ActualPos >= Len - 1)
		{
			pString[Len - 1] = 0;
			Running = false;
			--pInStr;
		}
		else
		{
			if (Chr == '\n')
			{
				pString[ActualPos] = 0;
				Running = false;
			}
			else if (Chr == '\r')
			{
				ActualPos = 0;
				pString[ActualPos] = 0;
			}
			else if (Chr == 0)
			{
				pString[ActualPos] = 0;
				pInStr = NULL;
				Running = false;
			}
			else pString[ActualPos++] = Chr;
		}
	}
	while (Running);
	return pInStr;
}
//---------------------------------------------------------------------

inline CLineBuffer::CLineBuffer(): HeadLine(0), TailLine(0)
{
	pBuffer = (char*)n_calloc(MAX_LINE_CHARS, MAX_LINES);
	n_assert(pBuffer);
	for (int i = 0; i < MAX_LINES; ++i)
		Lines[i].Set(pBuffer + i * MAX_LINE_CHARS, MAX_LINE_CHARS);
}
//---------------------------------------------------------------------

inline void CLineBuffer::Put(const char* pString)
{
	const char* pCurrStr = pString;
	while (pCurrStr && *pCurrStr && (pCurrStr = Lines[HeadLine].Add(pCurrStr)))
	{
		// Line ends with a newline (\n) so switch to the next line
		HeadLine = NextLine(HeadLine);
		if (HeadLine == TailLine)
			TailLine = NextLine(TailLine);
		Lines[HeadLine].Reset();
	}
}
//---------------------------------------------------------------------

// Fills the user provided char pointer OutLines with pointers to the N
// latest lines in the pString buffer. Return number of valid lines.
inline int CLineBuffer::GetLines(const char** OutLines, int MaxCount) const
{
	int Count = 0;
	for (int i = GetHeadLine(); (i != -1) && (Count < MaxCount); i = GetPrevLine(i))
	{
		const char* pLine = GetLine(i);
		if (!pLine) break;
		if (*pLine) OutLines[Count++] = pLine;
	}
	return Count;
}
//---------------------------------------------------------------------

#endif
