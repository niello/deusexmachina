#ifndef N_LINEBUFFER_H
#define N_LINEBUFFER_H

#include "kernel/ntypes.h"

// A ring buffer for strings.
// (C) 2002 RadonLabs GmbH

class nLineBuffer
{
private:

	struct nLine
	{
		int		line_len;
		int		act_pos;
		char*	line;

		nLine(): line(0), line_len(0), act_pos(0) {}

		void		Set(char* p, int len);
		void		Reset() { n_assert(line); line[0] = 0; act_pos = 0; }
		const char*	Append(const char* s);
	};

	enum
	{
		N_LINE_LEN  = 80,
		N_NUM_LINES = 256,
	};

	char*	pBuffer;
	nLine	Lines[N_NUM_LINES];
	int		TailLine;
	int		HeadLine;

	int NextLine(int l) const { return (++l >= N_NUM_LINES) ? 0 : l; }
	int PrevLine(int l) const { return (--l < 0) ? N_NUM_LINES - 1 : l; }

public:

	nLineBuffer();
	~nLineBuffer() { if (pBuffer) n_free(pBuffer); }

	void		Put(const char*);
	const char*	GetLine(uint Idx) const { n_assert(Idx < N_NUM_LINES); return Lines[Idx].line; }
	int			GetHeadLine() const { return HeadLine; }
	int			GetTailLine() const { return TailLine; }
	int			GetNextLine(int Idx) const { return (Idx == HeadLine) ? -1 : NextLine(Idx); }
	int			GetPrevLine(int Idx) const { return (Idx == TailLine) ? -1 : PrevLine(Idx); }
	int			GetLines(const char** OutLines, int MaxCount) const;
};

inline void nLineBuffer::nLine::Set(char* p, int len)
{
	n_assert(p);
	line = p;
	line[0] = 0;
	line_len = len;
	act_pos = 0;
}
//---------------------------------------------------------------------

// Appends string until line full, new line or end of string is reached.
// If new line, a pointer to the next char is returned, otherwise NULL.
// A '\r' in the string rewinds the cursor to the start of the line. If the
// string buffer is full, a 0 is appended in any case. Newlines are not copied.
inline const char* nLineBuffer::nLine::Append(const char* s)
{
	n_assert(s);

	char c;
	bool running = true;
	do
	{
		// Zeile voll?
		c = *s++;
		if (act_pos >= (line_len-1))
		{
			line[line_len - 1] = 0;
			running = false;
			--s;
		}
		else
		{
			if (c == '\n')
			{
				line[act_pos] = 0;
				running = false;
			}
			else if (c == '\r')
			{
				act_pos = 0;
				line[act_pos] = 0;
			}
			else if (c == 0)
			{
				line[act_pos] = 0;
				s = NULL;
				running = false;
			}
			else line[act_pos++] = c;
		}
	}
	while (running);
	return s;
}
//---------------------------------------------------------------------

inline nLineBuffer::nLineBuffer()
{
	pBuffer = (char*)n_calloc(N_LINE_LEN, N_NUM_LINES);
	n_assert(pBuffer);
	for (int i = 0; i < N_NUM_LINES; i++)
		Lines[i].Set((pBuffer + i*N_LINE_LEN), N_LINE_LEN);
	TailLine = 0;
	HeadLine = 0;
}
//---------------------------------------------------------------------

inline void nLineBuffer::Put(const char* s)
{
	const char *cont = s;
	while (cont && (*cont) && (cont = Lines[HeadLine].Append(cont)))
	{
		// Line ends with a newline (\n) so switch to next line
		HeadLine = NextLine(HeadLine);
		if (HeadLine == TailLine)
			TailLine = NextLine(TailLine);
		Lines[HeadLine].Reset();
	}
}
//---------------------------------------------------------------------

// Fills the user provided char pointer OutLines with pointers to the N
// latest lines in the line buffer. Return number of valid lines.
inline int nLineBuffer::GetLines(const char** OutLines, int MaxCount) const
{
	int Count = 0;
	for (int i = GetHeadLine(); (i != -1) && (Count < MaxCount); i = GetPrevLine(i))
	{
		const char* l = GetLine(i);
		if (!l) break;
		if (*l) OutLines[Count++] = l;
	}
	return Count;
}
//---------------------------------------------------------------------

#endif
