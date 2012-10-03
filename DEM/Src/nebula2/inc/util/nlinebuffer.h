#ifndef N_LINEBUFFER_H
#define N_LINEBUFFER_H
//------------------------------------------------------------------------------
/**
    @class nLineBuffer
    @ingroup NebulaDataTypes

    @brief A ring buffer for strings.

    (C) 2002 RadonLabs GmbH
*/
#include "kernel/ntypes.h"

//------------------------------------------------------------------------------
class nLineBuffer
{
public:
    /// constructor
    nLineBuffer();
    /// destructor
    ~nLineBuffer();
    /// write string to line buffer
    void Put(const char*);
    /// return pointer to line defined by line number
    const char* GetLine(int) const;
    /// get line number of first line
    int GetHeadLine() const;
    /// get line number of last line
    int GetTailLine() const;
    /// get line number of previous line
    int GetNextLine(int) const;
    /// get line number of next line
    int GetPrevLine(int) const;
    /// fill an user provided array with pointers to lines
    int GetLines(const char** array, int arraySize) const;

private:
    /// get next line number
    int nextLine(int) const;
    /// get previous line number
    int prevLine(int) const;

    /// private class to keep track of lines in buffer
    class nLine
    {
        friend class nLineBuffer;

        /// constructor
        nLine();
        /// set line pointer to external storage space
        void Set(char* p, int len);
        /// reset line pointer
        void Reset();
        /// append string to line
        const char* Append(const char* s);

        int line_len;
        int act_pos;
        char* line;
    };

    enum
    {
        N_LINE_LEN  = 80,
        N_NUM_LINES = 256,
    };
    char* c_buf;
    nLine line_array[N_NUM_LINES];
    int tail_line;
    int head_line;

};

//------------------------------------------------------------------------------
/**
*/
inline
nLineBuffer::nLine::nLine() :
    line(0),
    line_len(0),
    act_pos(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nLineBuffer::nLine::Set(char* p, int len)
{
    n_assert(p);
    this->line = p;
    this->line[0] = 0;
    this->line_len = len;
    this->act_pos = 0;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nLineBuffer::nLine::Reset()
{
    n_assert(this->line);
    this->line[0] = 0;
    this->act_pos = 0;
}

//------------------------------------------------------------------------------
/**
    Appends string until line full, new line or end of string is reached.

    If new line, a pointer to the next char is returned, otherwise NULL.
    A '\r' in the string rewinds the cursor to the start of the line. If
    the string buffer is full, a 0 is appended in any case. Newlines are
    not copied.
*/
inline
const char*
nLineBuffer::nLine::Append(const char* s)
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
            s--;
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
                act_pos       = 0;
                line[act_pos] = 0;
            }
            else if (c == 0)
            {
                line[act_pos] = 0;
                s = NULL;
                running = false;
            }
            else
            {
                line[act_pos++] = c;
            }
        }
    } while (running);
    return s;
}

//------------------------------------------------------------------------------
/**
*/
inline
nLineBuffer::nLineBuffer()
{
    int i;
    this->c_buf = (char*)n_calloc(N_LINE_LEN, N_NUM_LINES);
    n_assert(this->c_buf);
    for (i = 0; i < N_NUM_LINES; i++)
    {
        this->line_array[i].Set((this->c_buf + i*N_LINE_LEN), N_LINE_LEN);
    }
    this->tail_line = 0;
    this->head_line = 0;
}

//------------------------------------------------------------------------------
/**
*/
inline
nLineBuffer::~nLineBuffer()
{
    if (this->c_buf)
    {
        n_free(this->c_buf);
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
int nLineBuffer::nextLine(int l) const
{
    l++;
    if (l >= N_NUM_LINES) l = 0;
    return l;
}

//------------------------------------------------------------------------------
/**
*/
inline
int nLineBuffer::prevLine(int l) const
{
    l--;
    if (l < 0) l = N_NUM_LINES - 1;
    return l;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nLineBuffer::Put(const char* s)
{
    const char *cont = s;
    while (cont && (*cont) && (cont = this->line_array[this->head_line].Append(cont)))
    {
        // Line ends with a newline (\n) so switch to next line
        this->head_line = this->nextLine(this->head_line);
        if (this->head_line == this->tail_line)
        {
            this->tail_line = this->nextLine(this->tail_line);
        }
        this->line_array[this->head_line].Reset();
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
const char*
nLineBuffer::GetLine(int l) const
{
    n_assert(l >= 0);
    n_assert(l < N_NUM_LINES);
    return this->line_array[l].line;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nLineBuffer::GetHeadLine() const
{
    return this->head_line;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nLineBuffer::GetTailLine() const
{
    return this->tail_line;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nLineBuffer::GetNextLine(int l) const
{
    if (l == this->head_line) return -1;
    return nextLine(l);
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nLineBuffer::GetPrevLine(int l) const
{
    if (l == this->tail_line) return -1;
    return prevLine(l);
}

//------------------------------------------------------------------------------
/**
    Fills the user provided char pointer array with pointers to the N
    latest lines in the line buffer. Return number of valid lines.

    @param  array       an char pointer array to fill with pointers
    @param  arraySize   the size of the char pointer array
    @return             number of valid lines
*/
inline
int
nLineBuffer::GetLines(const char** array, int arraySize) const
{
    int i;
    int numLines = 0;
    for (i = this->GetHeadLine();
         (i != -1) && (numLines < arraySize);
         i = this->GetPrevLine(i))
    {
        const char* l = this->GetLine(i);
        if (l)
        {
            if (*l) array[numLines++] = l;
        }
        else break;
    }
    return numLines;
}

//------------------------------------------------------------------------------
#endif
