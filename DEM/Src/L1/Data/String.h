#ifndef N_STRING_H
#define N_STRING_H

#include <Data/Array.h>
#include <Data/Hash.h>
#include <Math/Matrix44.h> //!!!remove dependent code to utils
#include <Core/Core.h>
#include <stdarg.h>

// Character string with local buffer for small strings to avoid allocations

class CString
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

	void	SetLength(int NewLen) { if (pString) Len = NewLen; else LocalLen = NewLen; }
	char*	GetLastDirSeparator() const;

public:

	static const CString Empty;

	CString(): pString(NULL), Len(0), LocalLen(0) {}
	CString(const char* pSrc): pString(NULL), Len(0), LocalLen(0) { Set(pSrc); }
	CString(const char* pSrc, uint SrcLength): pString(NULL), Len(0), LocalLen(0) { Set(pSrc, SrcLength); }
	CString(const CString& Other): pString(NULL), Len(0) { pLocalString[0] = 0; Set(Other.CStr(), Other.Length()); }
	~CString() { Clear(); }

	void			Set(const char* pSrc, int SrcLength);
	void			Set(const char* pSrc) { Set(pSrc, pSrc ? (int)strlen(pSrc) : 0); }
	void			Reserve(DWORD NewLength);
	void			Add(char Chr) { AppendRange(&Chr, 1); }
	void			Add(const char* str, int Len = -1) { n_assert(str); AppendRange(str, Len == -1 ? strlen(str) : Len); }
	void			Add(const CString& Str) { AppendRange(Str.CStr(), Str.Length()); }
	void			AppendRange(const char* str, uint CharCount);
	void			AppendInt(int val) { CString str; str.SetInt(val); Add(str); }
	void			AppendFloat(float val) { CString str; str.SetFloat(val); Add(str); }
	void			Clear();

	static CString	Concatenate(const CArray<CString>& Strings, const CString& WhiteSpace);
	int				Tokenize(const char* SplitChars, uchar Fence, CArray<CString>& Tokens) const;
	CString			SubString(DWORD From, DWORD CharCount = 0) const;
	void			Strip(DWORD Idx);
	void			Strip(const char* CharSet);
	int				FindStringIndex(const CString& Str, DWORD StartIdx = 0) const;
	int				FindCharIndex(unsigned char Chr, DWORD StartIdx = 0) const;
	bool			ContainsCharFromSet(const char* CharSet) const { return CharSet && !!strpbrk(CStr(), CharSet); }
	bool			ContainsOnly(const CString& CharSet) const;
	void			ToLower();
	void			ToUpper();
	CString			Trim(const char* CharSet = DEM_WHITESPACE, bool Left = true, bool Right = true) const;
	void			TrimInplace(const char* CharSet = DEM_WHITESPACE, bool Left = true, bool Right = true);
	CString			Replace(const char* str, const char* pReplaceWith) const;
	void			Replace(char c, char subst);
	void			ReplaceChars(const char* CharSet, char replacement);

	const char*		GetExtension() const;
	bool			CheckExtension(const char* Extension) const;
	void			StripExtension();
	void			ConvertBackslashes() { Replace('\\', '/'); }
	void			ReplaceIllegalFilenameChars(char ReplaceWith) { ReplaceChars("\\/:*?\"<>|", ReplaceWith); }
	void			StripTrailingSlash();
	CString			ExtractFileName() const;
	CString			ExtractLastDirName() const;
	CString			ExtractDirName() const;
	CString			ExtractToLastSlash() const;

	bool			MatchPattern(const CString& Pattern) const { return n_strmatch(CStr(), Pattern.CStr()); }

	void __cdecl	Format(const char* pFormatStr, ...) __attribute__((format(printf,2,3)));
	void			FormatWithArgs(const char* pFormatStr, va_list args);

	void			UTF8toANSI();
	void			ANSItoUTF8();

	const char*		CStr() const { return pString ? pString : pLocalString; }
	DWORD			Length() const { return pString ? Len : LocalLen; }
	bool			IsEmpty() const { return !IsValid(); }
	bool			IsValid() const { return (pString && *pString) || *pLocalString; }

	void			SetInt(int val) { Format("%d", val); }
	void			SetFloat(float val) { Format("%.6f", val); }
	void			SetBool(bool val) { *this = val ? "true" : "false"; }
	void			SetVector3(const vector3& v) { Format("%.6f,%.6f,%.6f", v.x, v.y, v.z); }
	void			SetVector4(const vector4& v) { Format("%.6f,%.6f,%.6f,%.6f", v.x, v.y, v.z, v.w); }
	void			SetMatrix44(const matrix44& m);

	static CString	FromInt(int i) { CString str; str.SetInt(i); return str; }
	static CString	FromFloat(float f) { CString str; str.SetFloat(f); return str; }
	static CString	FromBool(bool b) { CString str; str.SetBool(b); return str; }
	static CString	FromVector3(const vector3& v) { CString str; str.SetVector3(v); return str; }
	static CString	FromVector4(const vector4& v) { CString str; str.SetVector4(v); return str; }
	static CString	FromMatrix44(const matrix44& m) { CString str; str.SetMatrix44(m); return str; }

	bool			IsValidInt() const { return ContainsOnly(" \t-+01234567890"); }
	bool			IsValidFloat() const { return ContainsOnly(" \t-+.e1234567890"); }
	bool			IsValidBool() const;
	bool			IsValidVector3() const { return ContainsOnly(" \t-+.,e1234567890"); }
	bool			IsValidVector4() const { return ContainsOnly(" \t-+.,e1234567890"); }
	bool			IsValidMatrix44() const { return ContainsOnly(" \t-+.,e1234567890"); }

	int				AsInt() const { return atoi(CStr()); }
	float			AsFloat() const { return (float)atof(CStr()); }
	bool			AsBool() const;

	int				GetLastDirSeparatorIndex() const { char* pSep = GetLastDirSeparator(); return pSep ? pSep - CStr() : -1; }

	CString&		operator =(const CString& Other) { if (&Other != this) Set(Other.CStr(), Other.Length()); return *this; }
	CString&		operator =(const char* pStr) { if (pStr != CStr()) Set(pStr); return *this; }
	CString&		operator +=(char Chr) { Add(Chr); return *this; }
	CString&		operator +=(const char* pStr) { Add(pStr); return *this; }
	CString&		operator +=(const CString& Other) { Add(Other); return *this; }
	char			operator [](int i) const { n_assert(i >= 0 && i <= (int)Length()); return CStr()[i]; }
	char&			operator [](int i) { n_assert(i >= 0 && i <= (int)Length()); return pString ? pString[i] : pLocalString[i]; }

	friend bool		operator ==(const CString& a, const CString& b) { return a.Length() == b.Length() && !strcmp(a.CStr(), b.CStr()); }
	friend bool		operator ==(const CString& a, const char* b) { return b ? (!strcmp(a.CStr(), b)) : a.IsEmpty(); }
	friend bool		operator !=(const CString& a, const CString& b) { return a.Length() != b.Length() || strcmp(a.CStr(), b.CStr()); }
	friend bool		operator !=(const CString& a, const char* b) { return b ? (!!strcmp(a.CStr(), b)) : a.IsValid(); }
	friend bool		operator <(const CString& a, const CString& b) { return strcmp(a.CStr(), b.CStr()) < 0; }
	friend bool		operator >(const CString& a, const CString& b) { return strcmp(a.CStr(), b.CStr()) > 0; }
	friend bool		operator <=(const CString& a, const CString& b) { return strcmp(a.CStr(), b.CStr()) <= 0; }
	friend bool		operator >=(const CString& a, const CString& b) { return strcmp(a.CStr(), b.CStr()) >= 0; }
};

template<> inline unsigned int Hash<CString>(const CString& Key)
{
	return Hash(Key.CStr(), Key.Length());
}
//---------------------------------------------------------------------

inline void CString::Clear()
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

inline void CString::ToLower()
{
	if (pString) _strlwr_s(pString, Len + 1);
	else _strlwr_s(pLocalString, LocalLen + 1);
}
//---------------------------------------------------------------------

inline void CString::ToUpper()
{
	if (pString) _strupr_s(pString, Len + 1);
	else _strupr_s(pLocalString, LocalLen + 1);
}
//---------------------------------------------------------------------

//!!!to tokenizer!
//Tokenize a pString, but keeps the pString within the fence-character
//intact. For instance for the sentence:
//He said: "I don't know."
//A Tokenize(" ", '"', Tokens) would return:
//token 0: He
//token 1: said:
//token 2: I don't know.
inline int CString::Tokenize(const char* SplitChars, uchar fence, CArray<CString>& Tokens) const
{
    // create a temporary pString, which will be destroyed during the operation
    CString str(*this);
    char* ptr = (char*)str.CStr();
    char* end = ptr + strlen(ptr);
    while (ptr < end)
    {
        char* c;

        // skip white space
        while (*ptr && strchr(SplitChars, *ptr)) ++ptr;

		if (*ptr)
        {
            // check for fenced area
            if ((fence == *ptr) && (c = strchr(++ptr, fence)))
            {
                *c++ = 0;
                Tokens.Add(ptr);
                ptr = c;
            }
            else if ((c = strpbrk(ptr, SplitChars)))
            {
                *c++ = 0;
                Tokens.Add(ptr);
                ptr = c;
            }
            else
            {
                Tokens.Add(ptr);
                break;
            }
        }
    }
    return Tokens.GetCount();
}
//---------------------------------------------------------------------

inline CString CString::SubString(DWORD From, DWORD CharCount) const
{
	DWORD StrLen = Length();
	if (From > StrLen) From = StrLen;
	if (!CharCount || From + CharCount > StrLen) CharCount = StrLen - From;
	return CString(CStr() + From, CharCount);
}
//---------------------------------------------------------------------

inline void CString::Strip(DWORD Idx)
{
	n_assert(Idx < (DWORD)Length());
	char* pStr = pString ? pString : pLocalString;
	pStr[Idx] = 0;
	SetLength(Idx);
}
//---------------------------------------------------------------------

// Terminates the string at the first occurrence of one of the characters in a CharSet
inline void CString::Strip(const char* CharSet)
{
	n_assert(CharSet);
	char* pStr = pString ? pString : pLocalString;
	char* pOccur = strpbrk(pStr, CharSet);
	if (pOccur)
	{
		*pOccur = 0;
		SetLength(pOccur - pStr);
	}
}
//---------------------------------------------------------------------

// Strips last slash, if the path name ends on a slash
inline void CString::StripTrailingSlash()
{
	if (!Length()) return;
	int Pos = Length() - 1;
	char* pStr = pString ? pString : pLocalString;
	if (pStr[Pos] == '/' || pStr[Pos] == '\\')
	{
		pStr[Pos] = 0;
		if (pString) --Len;
		else --LocalLen;
	}
}
//---------------------------------------------------------------------

inline CString CString::Trim(const char* CharSet, bool Left, bool Right) const
{
	CString Str(*this);
	Str.TrimInplace(CharSet, Left, Right);
	return Str;
}
//---------------------------------------------------------------------

inline int CString::FindCharIndex(uchar Chr, DWORD StartIdx) const
{
	DWORD StrLen = Length();
	if (!StrLen) return INVALID_INDEX;
	n_assert(StartIdx < StrLen);
	const char* pStr = CStr();
	const char* pChar = strchr(pStr + StartIdx, Chr);
	return pChar ? pChar - pStr : INVALID_INDEX;
}
//---------------------------------------------------------------------

inline void CString::Replace(char Char, char NewChar)
{
	char* pChar = pString ? pString : pLocalString;
	while (*pChar)
	{
		if (*pChar == Char) *pChar = NewChar;
		++pChar;
	}
}
//---------------------------------------------------------------------

inline void CString::ReplaceChars(const char* CharSet, char NewChar)
{
	n_assert(CharSet);
	char* pChar = pString ? pString : pLocalString;
	while (*pChar)
	{
		if (strchr(CharSet, *pChar)) *pChar = NewChar;
		++pChar;
	}
}
//---------------------------------------------------------------------

//!!!TO UTILS!
// This converts an UTF-8 pString to 8-bit-ANSI. Note that only characters in the
// range 0 .. 255 are converted, all other characters will be converted to a question mark.
// For conversion rules see http://www.cl.cam.ac.uk/~mgk25/unicode.html#utf-8
inline void CString::UTF8toANSI()
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
//---------------------------------------------------------------------

//!!!TO UTILS!
inline void CString::ANSItoUTF8()
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
        if (c < 128) *dstPtr++ = c;
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

//!!!TO UTILS! FS/OS dependent Path utils!
// Returns pointer to extension (without the dot), or NULL
inline const char* CString::GetExtension() const
{
	char* pLastDirSep = GetLastDirSeparator();
	const char* pStr = strrchr(pLastDirSep ? pLastDirSep + 1 : CStr(), '.');
	return (pStr && *(++pStr)) ? pStr : NULL;
}
//---------------------------------------------------------------------

// Extension must be without the dot
inline bool CString::CheckExtension(const char* Extension) const
{
	n_assert(Extension);
	const char* pExt = GetExtension();
	return pExt && !n_stricmp(Extension, pExt);
}
//---------------------------------------------------------------------

inline void CString::StripExtension()
{
	char* ext = (char*)GetExtension();
	if (ext) ext[-1] = 0;
	SetLength(strlen(CStr()));
}
//---------------------------------------------------------------------

// Get a pointer to the last directory separator.
inline char* CString::GetLastDirSeparator() const
{
	char* pStr = (char*)CStr();
	char* lastSlash = strrchr(pStr, '/');
	if (!lastSlash) lastSlash = strrchr(pStr, '\\');
	if (!lastSlash) lastSlash = strrchr(pStr, ':');
	return lastSlash;
}
//---------------------------------------------------------------------

inline CString CString::ExtractFileName() const
{
	char* pLastDirSep = GetLastDirSeparator();
	CString Path = pLastDirSep ? pLastDirSep + 1 : CStr();
	return Path;
}
//---------------------------------------------------------------------

// Return a CString object containing the last directory of the path, i.e. a category
inline CString CString::ExtractLastDirName() const
{
    CString pathString(*this);
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
                return CString(secLastSlash+1);
            }
        }
    }
    return "";
}
//---------------------------------------------------------------------

// Return a CString object containing the part before the last directory separator.
// NOTE (floh): I left my fix in that returns the last slash (or colon), this was
// necessary to tell if a dirname is a normal directory or an assign.
inline CString CString::ExtractDirName() const
{
	CString Path(*this);
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
inline CString CString::ExtractToLastSlash() const
{
	CString Path(*this);
	char* pLastDirSep = Path.GetLastDirSeparator();
	if (pLastDirSep) pLastDirSep[1] = 0;
	else Path = "";
	return Path;
}
//---------------------------------------------------------------------

inline bool CString::ContainsOnly(const CString& CharSet) const
{
	const char* pStr = CStr();
	DWORD StrLen = Length();
	for (DWORD i = 0; i < StrLen; ++i)
		if (CharSet.FindCharIndex(pStr[i]) == INVALID_INDEX) FAIL;
	OK;
}
//---------------------------------------------------------------------

inline void __cdecl CString::Format(const char* pFormatStr, ...)
{
	va_list ArgList;
	va_start(ArgList, pFormatStr);
	FormatWithArgs(pFormatStr, ArgList);
	va_end(ArgList);
}
//---------------------------------------------------------------------

inline void CString::FormatWithArgs(const char* pFormatStr, va_list Args)
{
	va_list ArgList;
	va_copy(ArgList, Args);
	size_t ReqLength;
	ReqLength = _vscprintf(pFormatStr, ArgList) + 1; // + 1 for terminating \0
	va_end(ArgList);

	Clear();
	if (ReqLength >= LOCAL_STRING_SIZE)
	{
		pString = (char*)n_malloc(ReqLength);
		_vsnprintf_s(pString, ReqLength, ReqLength, pFormatStr, Args);
		--ReqLength;
		pString[ReqLength] = 0;
		Len = ReqLength;
	}
	else
	{
		_vsnprintf_s(pLocalString, ReqLength, ReqLength, pFormatStr, Args);
		--ReqLength;
		pLocalString[ReqLength] = 0;
		LocalLen = (ushort)ReqLength;
	}
}
//---------------------------------------------------------------------

inline CString CString::Concatenate(const CArray<CString>& Strings, const CString& WhiteSpace)
{
	CString Result;
	for (int i = 0; i < Strings.GetCount(); i++)
	{
		Result.Add(Strings[i]);
		if (i < Strings.GetCount() - 1)
			Result.Add(WhiteSpace);
	}
	return Result;
}
//---------------------------------------------------------------------

//???to utils?
inline void CString::SetMatrix44(const matrix44& m)
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

//???to utils?
inline bool CString::IsValidBool() const
{
	static const char* Bools[] = { "no", "yes", "off", "on", "false", "true", NULL };
	int i = 0;
	while (Bools[i])
	{
		if (!n_stricmp(Bools[i], CStr())) OK;
		++i;
	}
	FAIL;
}
//---------------------------------------------------------------------

//???to utils?
inline bool CString::AsBool() const
{
	static const char* Bools[] = { "no", "yes", "off", "on", "false", "true", NULL };
	int i = 0;
	while (Bools[i])
	{
		if (!n_stricmp(Bools[i], CStr())) return (i & 1);
		++i;
	}
	n_assert2(false, "Invalid pString value for bool!");
	FAIL;
}
//---------------------------------------------------------------------

static inline CString operator +(const CString& Str1, const CString& Str2)
{
	CString Result(Str1);
	Result.Add(Str2.CStr());
	return Result;
}
//---------------------------------------------------------------------

#endif
