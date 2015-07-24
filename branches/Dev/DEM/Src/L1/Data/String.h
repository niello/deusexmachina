#pragma once
#ifndef DEM_L1_STRING_H
#define DEM_L1_STRING_H

#include <Data/Hash.h>
#include <System/System.h>
#include <stdarg.h>

// Character string. Set(SrcString) and CString(SrcString) copy data to newly allocated buffer.

class CString
{
protected:

	char*	pString;
	DWORD	Length;		// Used space, NOT including terminating '\0'
	DWORD	MaxLength;	// Allocated space, NOT including terminating '\0'

public:

	static const CString Empty;

	CString(): pString(NULL), Length(0), MaxLength(0) {}
	explicit CString(const char* pSrc);
	explicit CString(const char* pSrc, DWORD SrcLength, DWORD PreallocatedFreeSpace = 0);
	CString(const CString& Other, DWORD PreallocatedFreeSpace = 0): CString(Other.CStr(), Other.GetLength(), PreallocatedFreeSpace) { }
	~CString() { if (pString && MaxLength > 0) n_free(pString); }

	void			Reallocate(DWORD NewMaxLength);
	void			Reserve(DWORD Bytes) { if (Length + Bytes > MaxLength) Reallocate(Length + Bytes); }
	void			FreeUnusedMemory() { Reallocate(Length); }

	void			Set(const char* pSrc, DWORD SrcLength, DWORD PreallocatedFreeSpace = 0);
	void			Set(const char* pSrc) { Set(pSrc, pSrc ? strlen(pSrc) : 0); }
	void			Clear();
	void __cdecl	Format(const char* pFormatStr, ...) __attribute__((format(printf,2,3)));
	void			FormatWithArgs(const char* pFormatStr, va_list args);
	//???Wrap(const char* pSrc)? - wrap NULL-terminated string w/out ownership

	void			Add(const char* pStr, DWORD StrLength);
	void			Add(char Chr);
	void			Add(const char* pStr) { Add(pStr, strlen(pStr)); }
	void			Add(const CString& Str) { Add(Str.CStr(), Str.GetLength()); }
	//Insert
	//Remove idx + size / char / charset / substring
	//SetLength(DWORD NewLength)
	void			Trim(const char* CharSet = DEM_WHITESPACE, bool Left = true, bool Right = true);

	void			ToLower() { if (pString) _strlwr_s(pString, Length + 1); }
	void			ToUpper() { if (pString) _strupr_s(pString, Length + 1); }

	const char*		CStr() const { return pString; }
	DWORD			GetLength() const { return Length; }
	bool			IsEmpty() const { return !pString || !*pString; }
	bool			IsValid() const { return pString && *pString; }

	operator		const char*() const { return pString; }
	operator		bool() const { return pString && *pString; }

	bool			operator ==(const char* pOther) const { return !strcmp(pString, pOther); }
	bool			operator ==(const CString& Other) const { return Length == Other.Length && !strcmp(pString, Other.pString); }
	bool			operator !=(const char* pOther) const { return !!strcmp(pString, pOther); }
	bool			operator !=(const CString& Other) const { return Length != Other.Length || strcmp(pString, Other.pString); }
	bool			operator >(const char* pOther) const { return strcmp(pString, pOther) > 0; }
	bool			operator <(const char* pOther) const { return strcmp(pString, pOther) < 0; }
	bool			operator >=(const char* pOther) const { return strcmp(pString, pOther) >= 0; }
	bool			operator <=(const char* pOther) const { return strcmp(pString, pOther) <= 0; }

	CString&		operator =(const char* pSrc) { Set(pSrc); return *this; }
	CString&		operator =(const CString& Src) { Set(Src); return *this; }
	CString&		operator +=(char Chr) { Add(Chr); return *this; }
	CString&		operator +=(const char* pStr) { Add(pStr); return *this; }
	CString&		operator +=(const CString& Other) { Add(Other); return *this; }
	char			operator [](int i) const { n_assert_dbg(i >= 0 && i <= (int)Length); return pString[i]; }
	char&			operator [](int i) { n_assert_dbg(i >= 0 && i <= (int)Length); return pString[i]; }
};

template<> inline unsigned int Hash<CString>(const CString& Key)
{
	return Hash(Key.CStr(), Key.GetLength());
}
//---------------------------------------------------------------------

inline CString::CString(const char* pSrc)
{
	if (pSrc)
	{
		DWORD SrcLength = strlen(pSrc);
		if (SrcLength)
		{
			pString = (char*)n_malloc(SrcLength + 1);
			memmove(pString, pSrc, SrcLength + 1);
			MaxLength = SrcLength;
			Length = SrcLength;
			return;
		}
	}

	pString = NULL;
	MaxLength = 0;
	Length = 0;
}
//---------------------------------------------------------------------

inline CString::CString(const char* pSrc, DWORD SrcLength, DWORD PreallocatedFreeSpace)
{
	n_assert_dbg(pSrc != pString);

	if (pSrc && SrcLength)
	{
		MaxLength = SrcLength + PreallocatedFreeSpace;
		pString = (char*)n_malloc(MaxLength + 1);
		memmove(pString, pSrc, SrcLength);
		pString[SrcLength] = 0;
		Length = SrcLength;
	}
	else
	{
		pString = NULL;
		MaxLength = 0;
		Length = 0;
	}
}
//---------------------------------------------------------------------

inline void CString::Set(const char* pSrc, DWORD SrcLength, DWORD PreallocatedFreeSpace)
{
	if (pSrc && SrcLength)
	{
		MaxLength = SrcLength + PreallocatedFreeSpace;
		if (SrcLength > MaxLength)
			pString = (char*)n_realloc(pString, SrcLength + 1);
		memmove(pString, pSrc, SrcLength);
		pString[SrcLength] = 0;
		Length = SrcLength;
	}
	else Clear();
}
//---------------------------------------------------------------------

inline void CString::Clear()
{
	SAFE_FREE(pString);
	MaxLength = 0;
	Length = 0;
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

inline void CString::Add(const char* pStr, DWORD StrLength)
{
	if (!pStr || !StrLength) return;
	Reserve(StrLength);
	memmove(pString + Length, pStr, StrLength);
	Length += StrLength;
	pString[Length] = 0;
}
//---------------------------------------------------------------------

inline void CString::Add(char Chr)
{
	Reserve(1);
	pString[Length] = Chr;
	pString[++Length] = 0;
}
//---------------------------------------------------------------------

static inline CString operator +(const CString& Str1, const CString& Str2)
{
	CString Result(Str1, Str2.GetLength());
	Result.Add(Str2);
	return Result;
}
//---------------------------------------------------------------------

static inline CString operator +(const CString& Str1, const char* pStr2)
{
	if (pStr2)
	{
		DWORD Len2 = strlen(pStr2);
		CString Result(Str1, Len2);
		Result.Add(pStr2, Len2);
		return Result;
	}
	else return Str1;
}
//---------------------------------------------------------------------

static inline CString operator +(const char* pStr1, const CString& Str2)
{
	if (pStr1)
	{
		DWORD Len1 = strlen(pStr1);
		CString Result(Str2, Len1);
		Result.Add(pStr1, Len1);
		return Result;
	}
	else return Str2;
}
//---------------------------------------------------------------------

/*

inline CString CString::Trim(const char* CharSet, bool Left, bool Right) const
{
//!!!GetTrimmedSubstring(const char* pStr, const char* CharSet, bool Left, bool Right, OutStart, OutLength);!
	CString Str(*this);
	Str.Trim(CharSet, Left, Right);
	return Str;
}
//---------------------------------------------------------------------

class CString
{
public:

	void			AppendInt(int val) { CString str; str.SetInt(val); Add(str); }
	void			AppendFloat(float val) { CString str; str.SetFloat(val); Add(str); }

	static CString	Concatenate(const CArray<CString>& Strings, const CString& WhiteSpace);
	int				Tokenize(const char* SplitChars, uchar Fence, CArray<CString>& Tokens) const;
	CString			SubString(DWORD From, DWORD CharCount = 0) const;
	void			Strip(DWORD Idx);
	void			Strip(const char* CharSet);
	int				FindStringIndex(const CString& Str, DWORD StartIdx = 0) const;
	int				FindCharIndex(unsigned char Chr, DWORD StartIdx = 0) const;
	bool			ContainsCharFromSet(const char* CharSet) const { return CharSet && !!strpbrk(CStr(), CharSet); }
	bool			ContainsOnly(const CString& CharSet) const;
	CString			Replace(const char* str, const char* pReplaceWith) const;
	void			Replace(char c, char subst);
	void			ReplaceChars(const char* CharSet, char replacement);

	void			ConvertBackslashes() { Replace('\\', '/'); }
	void			ReplaceIllegalFilenameChars(char ReplaceWith) { ReplaceChars("\\/:*?\"<>|", ReplaceWith); }
	void			StripTrailingSlash();

	void			UTF8toANSI();
	void			ANSItoUTF8();

	bool			IsValidInt() const { return ContainsOnly(" \t-+01234567890"); }
	bool			IsValidFloat() const { return ContainsOnly(" \t-+.e1234567890"); }
	bool			IsValidBool() const;
	bool			IsValidVector3() const { return ContainsOnly(" \t-+.,e1234567890"); }
	bool			IsValidVector4() const { return ContainsOnly(" \t-+.,e1234567890"); }
	bool			IsValidMatrix44() const { return ContainsOnly(" \t-+.,e1234567890"); }
};

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

inline bool CString::ContainsOnly(const CString& CharSet) const
{
	const char* pStr = CStr();
	DWORD StrLen = Length();
	for (DWORD i = 0; i < StrLen; ++i)
		if (CharSet.FindCharIndex(pStr[i]) == INVALID_INDEX) FAIL;
	OK;
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
*/

#endif
