#ifndef N_STRING_H
#define N_STRING_H

#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>
#include <util/narray.h>
#include <mathlib/vector.h>
#include <mathlib/matrix.h>
#include <util/Hash.h>

// Character string with local buffer for small strings to avoid allocations

class nString
{
protected:

	enum { LOCAL_STRING_SIZE = 14 };

	char* pString;
	union
	{
		struct
		{
			char	pLocalString[LOCAL_STRING_SIZE];
			ushort	LocalLen;
		};
		uint Len;
	};

	void	SetLength(int NewLen);
	char*	GetLastDirSeparator() const;

public:

	static const nString Empty;

	nString(): pString(NULL), Len(0), LocalLen(0) {}
	nString(const char* pSrc): pString(NULL), Len(0), LocalLen(0) { Set(pSrc); }
	nString(const char* pSrc, uint SrcLength): pString(NULL), Len(0), LocalLen(0) { Set(pSrc, SrcLength); }
	nString(const nString& Other): pString(NULL), Len(0) { pLocalString[0] = 0; Set(Other.CStr(), Other.Length()); }
	~nString() { Clear(); }

	void			Set(const char* pSrc, int SrcLength);
	void			Set(const char* pSrc) { Set(pSrc, pSrc ? (int)strlen(pSrc) : 0); }
	void			Append(const char* str) { n_assert(str); AppendRange(str, strlen(str)); }
	void			Append(const nString& Str) { AppendRange(Str.CStr(), Str.Length()); }
	void			AppendRange(const char* str, uint CharCount);
	void			AppendInt(int val) { nString str; str.SetInt(val); Append(str); }
	void			AppendFloat(float val) { nString str; str.SetFloat(val); Append(str); }
	void			Clear();

	static nString	Concatenate(const nArray<nString>& strArray, const nString& whiteSpace);
	int				Tokenize(const char* whiteSpace, nArray<nString>& Tokens) const;
	int				Tokenize(const char* whiteSpace, uchar fence, nArray<nString>& Tokens) const;
	nString			SubString(int from, int CharCount) const;
	void			Strip(const char* CharSet);
	int				FindStringIndex(const nString& v, int StartIdx) const;
	int				FindCharIndex(unsigned char c, int StartIdx) const;
	void			TerminateAtIndex(int Idx);
	bool			ContainsCharFromSet(const char* CharSet) const { return CharSet && !!strpbrk(CStr(), CharSet); }
	bool			ContainsOnly(const nString& CharSet) const;
	void			ToLower();
	void			ToUpper();
	nString			TrimLeft(const char* CharSet) const;
	nString			TrimRight(const char* CharSet) const;
	nString			Trim(const char* CharSet) const { return TrimLeft(CharSet).TrimRight(CharSet); }
	nString			Replace(const char* str, const char* pReplaceWith) const;
	void			Replace(char c, char subst);
	void			ReplaceChars(const char* CharSet, char replacement);

	const char*		GetExtension() const;
	bool			CheckExtension(const char* Extension) const;
	void			StripExtension();
	void			ConvertBackslashes() { Replace('\\', '/'); }
	void			ReplaceIllegalFilenameChars(char ReplaceWith) { ReplaceChars("\\/:*?\"<>|", ReplaceWith); }
	void			StripTrailingSlash();
	nString			ExtractFileName() const;
	nString			ExtractLastDirName() const;
	nString			ExtractDirName() const;
	nString			ExtractToLastSlash() const;

	bool			MatchPattern(const nString& Pattern) const { return n_strmatch(CStr(), Pattern.CStr()); }

	void __cdecl	Format(const char* pFormatStr, ...) __attribute__((format(printf,2,3)));
	void			FormatWithArgs(const char* pFormatStr, va_list args);

	void			UTF8toANSI();
	void			ANSItoUTF8();

	const char*		CStr() const;
	int				Length() const { return pString ? Len : LocalLen; }
	bool			IsEmpty() const { return !IsValid(); }
	bool			IsValid() const { return (pString && *pString) || *pLocalString; }

	void			SetInt(int val) { Format("%d", val); }
	void			SetFloat(float val) { Format("%.6f", val); }
	void			SetBool(bool val) { *this = val ? "true" : "false"; }
	void			SetVector3(const vector3& v) { Format("%.6f,%.6f,%.6f", v.x, v.y, v.z); }
	void			SetVector4(const vector4& v) { Format("%.6f,%.6f,%.6f,%.6f", v.x, v.y, v.z, v.w); }
	void			SetMatrix44(const matrix44& m);

	static nString	FromInt(int i) { nString str; str.SetInt(i); return str; }
	static nString	FromFloat(float f) { nString str; str.SetFloat(f); return str; }
	static nString	FromBool(bool b) { nString str; str.SetBool(b); return str; }
	static nString	FromVector3(const vector3& v) { nString str; str.SetVector3(v); return str; }
	static nString	FromVector4(const vector4& v) { nString str; str.SetVector4(v); return str; }
	static nString	FromMatrix44(const matrix44& m) { nString str; str.SetMatrix44(m); return str; }

	bool			IsValidInt() const { return ContainsOnly(" \t-+01234567890"); }
	bool			IsValidFloat() const { return ContainsOnly(" \t-+.e1234567890"); }
	bool			IsValidBool() const;
	bool			IsValidVector3() const { return ContainsOnly(" \t-+.,e1234567890"); }
	bool			IsValidVector4() const { return ContainsOnly(" \t-+.,e1234567890"); }
	bool			IsValidMatrix44() const { return ContainsOnly(" \t-+.,e1234567890"); }

	int				AsInt() const { return atoi(CStr()); }
	float			AsFloat() const { return (float)atof(CStr()); }
	bool			AsBool() const;
	vector3			AsVector3() const;
	vector4			AsVector4() const;
	matrix44		AsMatrix44() const;

	int				GetLastDirSeparatorIndex() const { return GetLastDirSeparator() - CStr(); }

	nString&		operator =(const nString& Other);
	nString&		operator =(const char* pStr) { if (pStr != CStr()) Set(pStr); return *this; }
	nString&		operator +=(const char* pStr) { Append(pStr); return *this; }
	nString&		operator +=(const nString& Other) { Append(Other); return *this; }
	char			operator [](int i) const;
	char&			operator [](int i);

	friend bool		operator ==(const nString& a, const nString& b) { return !strcmp(a.CStr(), b.CStr()); }
	friend bool		operator ==(const nString& a, const char* b) { return b ? (!strcmp(a.CStr(), b)) : a.IsEmpty(); }
	friend bool		operator !=(const nString& a, const nString& b) { return !!strcmp(a.CStr(), b.CStr()); }
	friend bool		operator !=(const nString& a, const char* b) { return b ? (!!strcmp(a.CStr(), b)) : a.IsValid(); }
	friend bool		operator <(const nString& a, const nString& b) { return strcmp(a.CStr(), b.CStr()) < 0; }
	friend bool		operator >(const nString& a, const nString& b) { return strcmp(a.CStr(), b.CStr()) > 0; }
	friend bool		operator <=(const nString& a, const nString& b) { return strcmp(a.CStr(), b.CStr()) <= 0; }
	friend bool		operator >=(const nString& a, const nString& b) { return strcmp(a.CStr(), b.CStr()) >= 0; }
};

template<> inline unsigned int Hash<nString>(const nString& Key)
{
	return Hash(Key.CStr(), Key.Length());
}
//---------------------------------------------------------------------

inline void nString::Set(const char* pSrc, int SrcLength)
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

inline void nString::Clear()
{
	if (pString)
	{
		n_free((void*)pString);
		pString = NULL;
	}
	pLocalString[0] = 0;
	LocalLen = 0;
}
//---------------------------------------------------------------------

////------------------------------------------------------------------------------
///**
//    Reserves internal space to prevent excessive heap re-allocations.
//    If you plan to do many Append() operations this may help alot.
//*/
//inline void
//String::Reserve(int newSize)
//{
//    if (newSize > heapBufferSize)
//    {
//        Realloc(newSize);
//    }
//}
//---------------------------------------------------------------------

inline const char* nString::CStr() const
{
	if (pString) return pString;
	if (pLocalString[0]) return pLocalString;
	return ""; //???NULL?
}
//---------------------------------------------------------------------

inline void nString::ToLower()
{
	char* pStr = (char*)(pString ? pString : pLocalString);
	if (!pStr) return;
	while (*pStr)
	{
		*pStr = tolower(*pStr);
		++pStr;
	}
}
//---------------------------------------------------------------------

inline void nString::ToUpper()
{
	char* pStr = (char*)(pString ? pString : pLocalString);
	if (!pStr) return;
	while (*pStr)
	{
		*pStr = toupper(*pStr);
		++pStr;
	}
}
//---------------------------------------------------------------------

inline int nString::Tokenize(const char* whiteSpace, nArray<nString>& Tokens) const
{
	int numTokens = 0;

	nString TmpString(*this);
	char* pStr = (char*)TmpString.CStr();
	const char* pTok;
	while (pTok = strtok(pStr, whiteSpace))
	{
		Tokens.Append(pTok);
		pStr = NULL;
		++numTokens;
	}
	return numTokens;
}
//---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
    Tokenize a pString, but keeps the pString within the fence-character
    intact. For instance for the sentence:

    He said: "I don't know."

    A Tokenize(" ", '"', Tokens) would return:

    token 0:    He
    token 1:    said:
    token 2:    I don't know.
*/
inline int nString::Tokenize(const char* whiteSpace, uchar fence, nArray<nString>& Tokens) const
{
    // create a temporary pString, which will be destroyed during the operation
    nString str(*this);
    char* ptr = (char*)str.CStr();
    char* end = ptr + strlen(ptr);
    while (ptr < end)
    {
        char* c;

        // skip white space
        while (*ptr && strchr(whiteSpace, *ptr)) ++ptr;

		if (*ptr)
        {
            // check for fenced area
            if ((fence == *ptr) && (c = strchr(++ptr, fence)))
            {
                *c++ = 0;
                Tokens.Append(ptr);
                ptr = c;
            }
            else if ((c = strpbrk(ptr, whiteSpace)))
            {
                *c++ = 0;
                Tokens.Append(ptr);
                ptr = c;
            }
            else
            {
                Tokens.Append(ptr);
                break;
            }
        }
    }
    return Tokens.GetCount();
}
//---------------------------------------------------------------------

inline nString nString::SubString(int from, int CharCount) const
{
	n_assert(from <= Length());
	n_assert((from + CharCount) <= Length());
	const char* str = CStr();
	nString newString;
	newString.Set(&(str[from]), CharCount);
	return newString;
}
//---------------------------------------------------------------------

// Terminates the pString at the first occurrence of one of the characters in CharSet.
inline void nString::Strip(const char* CharSet)
{
	n_assert(CharSet);
	char* str = (char*)CStr();
	char* ptr = strpbrk(str, CharSet);
	if (ptr) *ptr = 0;
	SetLength(strlen(str));
}
//---------------------------------------------------------------------

inline int nString::FindStringIndex(const nString& v, int StartIdx) const
{
	n_assert(StartIdx >= 0 && StartIdx <= Length() - 1);
	n_assert(!v.IsEmpty());

	for (int i = StartIdx; i < Length(); ++i)
	{
		if (Length() - i < v.Length()) break;
		if (!strncmp(&(CStr()[i]), v.CStr(), v.Length())) return i;
	}

	return -1;
}
//---------------------------------------------------------------------

inline int nString::FindCharIndex(uchar c, int StartIdx) const
{
	n_assert(StartIdx < Length());
	if (!Length()) return -1;
	const char* pStr = CStr();
	const char* pChar = strchr(pStr + StartIdx, c);
	return pChar ? pChar - pStr : -1;
}
//---------------------------------------------------------------------

inline void nString::TerminateAtIndex(int Idx)
{
	n_assert(Idx < Length());
	char* pStr = (char*)CStr();
	pStr[Idx] = 0;
	SetLength(strlen(pStr));
}
//---------------------------------------------------------------------

// Strips last slash, if the path name ends on a slash.
inline void nString::StripTrailingSlash()
{
	if (!Length()) return;
	int Pos = Length() - 1;
	char* pStr = (char*)CStr();
	if (pStr[Pos] == '/' || pStr[Pos] == '\\')
	{
		pStr[Pos] = 0;
		if (pString) --Len;
		else --LocalLen;
	}
	SetLength(strlen(CStr()));
}
//---------------------------------------------------------------------

inline void nString::Replace(char Char, char NewChar)
{
	char* pStr = (char*)CStr();
	for (int i = 0; i <= Length(); ++i)
		if (pStr[i] == Char)
			pStr[i] = NewChar;
}
//---------------------------------------------------------------------

inline void nString::ReplaceChars(const char* CharSet, char NewChar)
{
	n_assert(CharSet);
	char* pChar = (char*)CStr();
	while (*pChar)
	{
		if (strchr(CharSet, *pChar)) *pChar = NewChar;
		++pChar;
	}
}
//---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
    This converts an UTF-8 pString to 8-bit-ANSI. Note that only characters
    in the range 0 .. 255 are converted, all other characters will be converted
    to a question mark.

    For conversion rules see http://www.cl.cam.ac.uk/~mgk25/unicode.html#utf-8
*/
inline
void
nString::UTF8toANSI()
{
    uchar* src = (uchar*)CStr();
    uchar* dst = src;
    uchar c;
    while ((c = *src++))
    {
        if (c >= 0x80)
        {
            if ((c & 0xE0) == 0xC0)
            {
                // a 2 byte sequence with 11 bits of information
                ushort wide = ((c & 0x1F) << 6) | (*src++ & 0x3F);
                if (wide > 0xff)
                {
                    c = '?';
                }
                else
                {
                    c = (uchar) wide;
                }
            }
            else if ((c & 0xF0) == 0xE0)
            {
                // a 3 byte sequence with 16 bits of information
                c = '?';
                src += 2;
            }
            else if ((c & 0xF8) == 0xF0)
            {
                // a 4 byte sequence with 21 bits of information
                c = '?';
                src += 3;
            }
            else if ((c & 0xFC) == 0xF8)
            {
                // a 5 byte sequence with 26 bits of information
                c = '?';
                src += 4;
            }
            else if ((c & 0xFE) == 0xFC)
            {
                // a 6 byte sequence with 31 bits of information
                c = '?';
                src += 5;
            }
        }
        *dst++ = c;
    }
    *dst = 0;
}

//------------------------------------------------------------------------------
/**
    Convert contained ANSI pString to UTF-8 in place.
*/
inline
void
nString::ANSItoUTF8()
{
    n_assert(!IsEmpty());
    int bufSize = Length() * 2 + 1;
    char* buffer = n_new_array(char, bufSize);
    char* dstPtr = buffer;
    const char* srcPtr = CStr();
    unsigned char c;
    while ((c = *srcPtr++))
    {
        // note: this only covers the 2 cases that the character
        // is between 0 and 127 and between 128 and 255
        if (c < 128)
        {
            *dstPtr++ = c;
        }
        else
        {
            *dstPtr++ = 192 + (c / 64);
            *dstPtr++ = 128 + (c % 64);
        }
    }
    *dstPtr = 0;
    Set(buffer);
    n_delete_array(buffer);
}
//---------------------------------------------------------------------

// Returns pointer to extension (without the dot), or NULL
inline const char* nString::GetExtension() const
{
	char* pLastDirSep = GetLastDirSeparator();
	const char* pStr = strrchr(pLastDirSep ? pLastDirSep + 1 : CStr(), '.');
	return (pStr && *(++pStr)) ? pStr : NULL;
}
//---------------------------------------------------------------------

// Extension must be without the dot
inline bool nString::CheckExtension(const char* Extension) const
{
	n_assert(Extension);
	const char* pExt = GetExtension();
	return pExt && !stricmp(Extension, pExt);
}
//---------------------------------------------------------------------

inline void nString::StripExtension()
{
	char* ext = (char*)GetExtension();
	if (ext) ext[-1] = 0;
	SetLength(strlen(CStr()));
}
//---------------------------------------------------------------------

// Get a pointer to the last directory separator.
inline char* nString::GetLastDirSeparator() const
{
	char* pStr = (char*)CStr();
	char* lastSlash = strrchr(pStr, '/');
	if (!lastSlash) lastSlash = strrchr(pStr, '\\');
	if (!lastSlash) lastSlash = strrchr(pStr, ':');
	return lastSlash;
}
//---------------------------------------------------------------------

inline nString nString::ExtractFileName() const
{
	char* pLastDirSep = GetLastDirSeparator();
	nString Path = pLastDirSep ? pLastDirSep + 1 : CStr();
	return Path;
}
//---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
    Return a nString object containing the last directory of the path, i.e.
    a category.

    - 17-Feb-04     floh    fixed a bug when the path ended with a slash
*/
inline nString nString::ExtractLastDirName() const
{
    nString pathString(*this);
    char* lastSlash = pathString.GetLastDirSeparator();

    // special case if path ends with a slash
    if (lastSlash)
    {
        if (!lastSlash[1])
        {
            *lastSlash = 0;
            lastSlash = pathString.GetLastDirSeparator();
        }

        char* secLastSlash = 0;
        if (lastSlash)
        {
            *lastSlash = 0; // cut filename
            secLastSlash = pathString.GetLastDirSeparator();
            if (secLastSlash)
            {
                *secLastSlash = 0;
                return nString(secLastSlash+1);
            }
        }
    }
    return "";
}
//---------------------------------------------------------------------

// Return a nString object containing the part before the last directory separator.
// NOTE (floh): I left my fix in that returns the last slash (or colon), this was
// necessary to tell if a dirname is a normal directory or an assign.
inline nString nString::ExtractDirName() const
{
	nString Path(*this);
	char* pLastDirSep = Path.GetLastDirSeparator();

	if (pLastDirSep) // If path ends with a slash
	{
		if (!pLastDirSep[1])
		{
			*pLastDirSep = 0;
			pLastDirSep = Path.GetLastDirSeparator();
		}
		if (pLastDirSep) *++pLastDirSep = 0;
	}

	Path.SetLength(strlen(Path.CStr()));
	return Path;
}
//---------------------------------------------------------------------

// Return a path pString object which contains of the complete path up to the last slash.
// Returns an empty pString if there is no slash in the path.
inline nString nString::ExtractToLastSlash() const
{
	nString Path(*this);
	char* pLastDirSep = Path.GetLastDirSeparator();
	if (pLastDirSep) pLastDirSep[1] = 0;
	else Path = "";
	return Path;
}
//---------------------------------------------------------------------

inline bool nString::ContainsOnly(const nString& CharSet) const
{
	const char* pStr = CStr();
	for (int i = 0; i < Length(); i++)
		if (CharSet.FindCharIndex(pStr[i], 0) == -1) return false;
	return true;
}
//---------------------------------------------------------------------

inline void __cdecl nString::Format(const char* pFormatStr, ...)
{
	va_list ArgList;
	va_start(ArgList, pFormatStr);
	FormatWithArgs(pFormatStr, ArgList);
	va_end(ArgList);
}
//---------------------------------------------------------------------

inline void nString::FormatWithArgs(const char* pFormatStr, va_list Args)
{
	va_list ArgList;
	va_copy(ArgList, Args);
	size_t ReqLength;
	ReqLength = _vscprintf(pFormatStr, ArgList) + 1; // + 1 for terminating NULL char
	va_end(ArgList);

	char* pBuffer = (char*)alloca(ReqLength);
	vsnprintf(pBuffer, ReqLength, pFormatStr, Args);
	Set(pBuffer);
}
//---------------------------------------------------------------------

inline void nString::SetLength(int length)
{
	if (pString) Len = length;
	else LocalLen = length;
}
//---------------------------------------------------------------------

inline nString nString::Concatenate(const nArray<nString>& strArray, const nString& whiteSpace)
{
	nString Result;
	for (int i = 0; i < strArray.GetCount(); i++)
	{
		Result.Append(strArray[i]);
		if (i < strArray.GetCount() - 1)
			Result.Append(whiteSpace);
	}
	return Result;
}
//---------------------------------------------------------------------

inline void nString::SetMatrix44(const matrix44& m)
{
	Format(	"%.6f, %.6f, %.6f, %.6f, "
			"%.6f, %.6f, %.6f, %.6f, "
			"%.6f, %.6f, %.6f, %.6f, "
			"%.6f, %.6f, %.6f, %.6f",
			m.M11, m.M12, m.M13, m.M14,
			m.M21, m.M22, m.M23, m.M24,
			m.M31, m.M32, m.M33, m.M34,
			m.M41, m.M42, m.M43, m.M44);
}
//---------------------------------------------------------------------

inline bool nString::IsValidBool() const
{
	static const char* Bools[] = { "no", "yes", "off", "on", "false", "true", NULL };
	int i = 0;
	while (Bools[i])
	{
		if (!n_stricmp(Bools[i], CStr())) return true;
		++i;
	}
	return false;
}
//---------------------------------------------------------------------

inline bool nString::AsBool() const
{
	static const char* Bools[] = { "no", "yes", "off", "on", "false", "true", NULL };
	int i = 0;
	while (Bools[i])
	{
		if (!n_stricmp(Bools[i], CStr())) return (i & 1);
		++i;
	}
	n_assert2(false, "Invalid pString value for bool!");
	return false;
}
//---------------------------------------------------------------------

inline vector3 nString::AsVector3() const
{
	nArray<nString> Tokens;
	Tokenize(", \t", Tokens);
	n_assert(Tokens.GetCount() == 3);
	vector3 v(Tokens[0].AsFloat(), Tokens[1].AsFloat(), Tokens[2].AsFloat());
	return v;
}
//---------------------------------------------------------------------

inline vector4 nString::AsVector4() const
{
	nArray<nString> Tokens;
	Tokenize(", \t", Tokens);
	n_assert(Tokens.GetCount() == 4);
	vector4 v(Tokens[0].AsFloat(), Tokens[1].AsFloat(), Tokens[2].AsFloat(), Tokens[3].AsFloat());
	return v;
}
//---------------------------------------------------------------------

inline matrix44 nString::AsMatrix44() const
{
	nArray<nString> Tokens;
	Tokenize(", \t", Tokens);
	n_assert(Tokens.GetCount() == 16);
	matrix44 m(	Tokens[0].AsFloat(),  Tokens[1].AsFloat(),  Tokens[2].AsFloat(),  Tokens[3].AsFloat(),
				Tokens[4].AsFloat(),  Tokens[5].AsFloat(),  Tokens[6].AsFloat(),  Tokens[7].AsFloat(),
				Tokens[8].AsFloat(),  Tokens[9].AsFloat(),  Tokens[10].AsFloat(), Tokens[11].AsFloat(),
				Tokens[12].AsFloat(), Tokens[13].AsFloat(), Tokens[14].AsFloat(), Tokens[15].AsFloat());
	return m;
}
//---------------------------------------------------------------------

inline nString& nString::operator =(const nString& Other)
{
	if (&Other != this)
	{
		Clear();
		Set(Other.CStr(), Other.Length());
	}
	return *this;
}
//---------------------------------------------------------------------

inline char nString::operator [](int i) const
{
	n_assert(i >= 0 && i <= Length());
	return pString ? pString[i]: pLocalString[i];
}
//---------------------------------------------------------------------

inline char& nString::operator [](int i)
{
	n_assert(i >= 0 && i <= Length());
	return pString ? pString[i]: pLocalString[i];
}
//---------------------------------------------------------------------

static inline nString operator +(const nString& s0, const nString& s1)
{
	nString Result(s0);
	Result.Append(s1.CStr());
	return Result;
}
//---------------------------------------------------------------------

#endif
