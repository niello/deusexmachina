#pragma once
#include <Data/Hash.h>
#include <System/System.h>
#include <stdarg.h>

// Character string. Set(SrcString) and CString(SrcString) copy data to newly allocated buffer.

class CString
{
protected:

	char*	pString;
	UPTR	Length;		// Used space, NOT including terminating '\0'
	UPTR	MaxLength;	// Allocated space, NOT including terminating '\0'

public:

	static const CString Empty;

	CString(): pString(nullptr), Length(0), MaxLength(0) {}
	explicit CString(const char* pSrc);
	explicit CString(const char* pSrc, UPTR SrcLength, UPTR PreallocatedFreeSpace = 0);
	explicit CString(std::string_view Src) : CString(Src.data(), Src.size()) {}
	CString(const CString& Other, UPTR PreallocatedFreeSpace = 0): CString(Other.CStr(), Other.GetLength(), PreallocatedFreeSpace) { }
	~CString() { if (pString) n_free(pString); }

	void			Reallocate(UPTR NewMaxLength);
	void			Reserve(UPTR Bytes) { if (Length + Bytes > MaxLength) Reallocate(Length + Bytes); }
	void			FreeUnusedMemory() { Reallocate(Length); }

	void			Set(const char* pSrc, UPTR SrcLength, UPTR PreallocatedFreeSpace = 0);
	void			Set(const char* pSrc) { Set(pSrc, pSrc ? strlen(pSrc) : 0); }
	void			Set(const CString& Src) { Set(Src.CStr(), Src.GetLength()); }
	void			Clear();
	void __cdecl	Format(const char* pFormatStr, ...) __attribute__((format(printf,2,3)));
	void			FormatWithArgs(const char* pFormatStr, va_list args);
	//???Wrap(const char* pSrc)? - wrap nullptr-terminated string w/out ownership

	void			Add(const char* pStr, UPTR StrLength);
	void			Add(char Chr);
	void			Add(const char* pStr) { Add(pStr, strlen(pStr)); }
	void			Add(const CString& Str) { Add(Str.CStr(), Str.GetLength()); }
	//Insert
	//Remove idx + size / char / charset / substring
	//SetLength(UPTR NewLength)
	void			Trim(const char* CharSet = DEM_WHITESPACE, bool Left = true, bool Right = true);
	void			TruncateRight(UPTR CharCount);
	CString			SubString(UPTR Start) const;
	CString			SubString(UPTR Start, UPTR Size) const;

	void			Replace(char CurrChar, char NewChar);
	void			Replace(const char* pCurrCharSet, char NewChar);
	void			Replace(const char* pSubStr, const char* pReplaceWith);
	void			ToLower() { if (pString) _strlwr_s(pString, Length + 1); }
	void			ToUpper() { if (pString) _strupr_s(pString, Length + 1); }

	IPTR			FindIndex(char Chr, UPTR StartIdx = 0) const;
	IPTR			FindIndex(const char* pStr, UPTR StartIdx = 0) const;
	bool			ContainsAny(const char* pCharSet) const { return pCharSet && pString && !!strpbrk(pString, pCharSet); }
	bool			ContainsOnly(const char* pCharSet) const;

	const char*		CStr() const { return pString ? pString : ""; }
	UPTR			GetLength() const { return Length; }
	bool			IsEmpty() const { return !pString || !*pString; }
	bool			IsValid() const { return pString && *pString; }

	operator		const char*() const { return pString ? pString : ""; }
	operator		bool() const { return pString && *pString; }
	operator		std::string_view() const { return std::string_view(pString, Length); }

	//???simplify by using const "" instead of nullptr in CString::pString?
	bool			operator ==(const CString& Other) const { return Length == Other.Length && (pString == Other.pString || (pString && Other.pString && !strcmp(pString, Other.pString))); }
	bool			operator !=(const CString& Other) const { return Length != Other.Length || (pString != Other.pString && (!pString || !Other.pString || strcmp(pString, Other.pString))); }
	bool			operator >(const char* pOther) const { return pString && (!pOther || strcmp(pString, pOther) > 0); }
	bool			operator <(const char* pOther) const { return pOther && (!pString || strcmp(pString, pOther) < 0); }
	bool			operator >=(const char* pOther) const { return pString == pOther || (pString && (!pOther || strcmp(pString, pOther) >= 0)); }
	bool			operator <=(const char* pOther) const { return pString == pOther || (pOther && (!pString || strcmp(pString, pOther) <= 0)); }

	CString&		operator =(const char* pSrc) { Set(pSrc); return *this; }
	CString&		operator =(const CString& Src) { Set(Src); return *this; }
	CString&		operator +=(char Chr) { Add(Chr); return *this; }
	CString&		operator +=(const char* pStr) { Add(pStr); return *this; }
	CString&		operator +=(const CString& Other) { Add(Other); return *this; }
	char			operator [](int i) const { n_assert_dbg(i >= 0 && i <= (int)Length); return pString[i]; }
	char&			operator [](int i) { n_assert_dbg(i >= 0 && i <= (int)Length); return pString[i]; }
	//char			operator [](UPTR i) const { n_assert_dbg(i <= Length); return pString[i]; }
	//char&			operator [](UPTR i) { n_assert_dbg(i <= Length); return pString[i]; }
};

inline bool operator ==(const CString& Str1, const char* pStr2)
{
	return Str1.CStr() == pStr2 || (Str1.CStr() && pStr2 && !strcmp(Str1.CStr(), pStr2));
}
//---------------------------------------------------------------------

inline bool operator ==(const char* pStr1, const CString& Str2)
{
	return Str2 == pStr1;
}
//---------------------------------------------------------------------

inline bool operator !=(const CString& Str1, const char* pStr2)
{
	return !(Str1 == pStr2);
}
//---------------------------------------------------------------------

inline bool operator !=(const char* pStr1, const CString& Str2)
{
	return Str2 != pStr1;
}
//---------------------------------------------------------------------

inline bool operator ==(const CString& Str1, std::string_view Str2)
{
	return std::string_view{ Str1.CStr() } == Str2;
}
//---------------------------------------------------------------------

inline bool operator ==(std::string_view Str1, const CString& Str2)
{
	return Str2 == Str1;
}
//---------------------------------------------------------------------

inline bool operator !=(const CString& Str1, std::string_view Str2)
{
	return !(Str1 == Str2);
}
//---------------------------------------------------------------------

inline bool operator !=(std::string_view Str1, const CString& Str2)
{
	return Str2 != Str1;
}
//---------------------------------------------------------------------

namespace DEM::Utils
{

template<> inline uint32_t Hash<CString>(const CString& Key)
{
	return Hash(Key.CStr(), Key.GetLength());
}
//---------------------------------------------------------------------

}

inline CString::CString(const char* pSrc)
{
	if (pSrc)
	{
		UPTR SrcLength = strlen(pSrc);
		if (SrcLength)
		{
			pString = (char*)n_malloc(SrcLength + 1);
			memmove(pString, pSrc, SrcLength + 1);
			MaxLength = SrcLength;
			Length = SrcLength;
			return;
		}
	}

	pString = nullptr;
	MaxLength = 0;
	Length = 0;
}
//---------------------------------------------------------------------

inline CString::CString(const char* pSrc, UPTR SrcLength, UPTR PreallocatedFreeSpace)
{
	if (pSrc && SrcLength)
	{
		MaxLength = SrcLength + PreallocatedFreeSpace;
		pString = (char*)n_malloc(MaxLength + 1);
		strncpy_s(pString, MaxLength + 1, pSrc, SrcLength);
		Length = SrcLength;
	}
	else
	{
		pString = PreallocatedFreeSpace ? (char*)n_malloc(PreallocatedFreeSpace + 1) : nullptr;
		MaxLength = PreallocatedFreeSpace;
		Length = 0;
	}
}
//---------------------------------------------------------------------

inline void CString::Set(const char* pSrc, UPTR SrcLength, UPTR PreallocatedFreeSpace)
{
	if (pSrc && SrcLength)
	{
		UPTR ReqLength = SrcLength + PreallocatedFreeSpace;
		if (ReqLength > MaxLength)
			pString = (char*)n_realloc(pString, ReqLength + 1);
		memmove(pString, pSrc, SrcLength);
		pString[SrcLength] = 0;
		Length = SrcLength;
		MaxLength = ReqLength;
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

inline void CString::Add(const char* pStr, UPTR StrLength)
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

inline void CString::TruncateRight(UPTR CharCount)
{
	Length = static_cast<UPTR>(std::max<IPTR>(Length - CharCount, 0));
	pString[Length] = 0;
}
//---------------------------------------------------------------------

inline CString CString::SubString(UPTR Start) const
{
	if (Start >= Length) return CString();
	return CString(pString + Start);
}
//---------------------------------------------------------------------

inline CString CString::SubString(UPTR Start, UPTR Size) const
{
	if (!Size || Start >= Length) return CString();
	if (Start + Size > Length) Size = Length - Start;
	return CString(pString + Start, Size);
}
//---------------------------------------------------------------------

inline void CString::Replace(char CurrChar, char NewChar)
{
	if (!pString) return;
	char* pChar = pString;
	char Char = *pChar;
	while (Char)
	{
		if (Char == CurrChar) *pChar = NewChar;
		++pChar;
		Char = *pChar;
	}
}
//---------------------------------------------------------------------

inline void CString::Replace(const char* pCurrCharSet, char NewChar)
{
	if (!pCurrCharSet) return;
	char* pChar = pString;
	while (*pChar)
	{
		if (strchr(pCurrCharSet, *pChar)) *pChar = NewChar;
		++pChar;
	}
}
//---------------------------------------------------------------------

inline IPTR CString::FindIndex(char Chr, UPTR StartIdx) const
{
	if (StartIdx >= Length) return INVALID_INDEX;
	const char* pChar = strchr(pString + StartIdx, Chr);
	return pChar ? pChar - pString : INVALID_INDEX;
}
//---------------------------------------------------------------------

inline IPTR CString::FindIndex(const char* pStr, UPTR StartIdx) const
{
	if (StartIdx >= Length) return INVALID_INDEX;
	const char* pChar = strstr(pString + StartIdx, pStr);
	return pChar ? pChar - pString : INVALID_INDEX;
}
//---------------------------------------------------------------------

inline bool CString::ContainsOnly(const char* pCharSet) const
{
	if (!pCharSet) FAIL;
	const char* pCurr = pString;
	while (pCurr)
	{
		if (!strchr(pCharSet, *pCurr)) FAIL;
		++pCurr;
	}
	OK;
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
		UPTR Len2 = strlen(pStr2);
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
		CString Result(pStr1, strlen(pStr1), Str2.GetLength());
		Result.Add(Str2);
		return Result;
	}
	else return Str2;
}
//---------------------------------------------------------------------

static inline CString operator +(const CString& Str, char Char)
{
	if (Char)
	{
		CString Result(Str, 1);
		Result.Add(Char);
		return Result;
	}
	else return Str;
}
//---------------------------------------------------------------------

static inline CString operator +(char Char, const CString& Str)
{
	if (Char)
	{
		CString Result(&Char, 1, Str.GetLength());
		Result.Add(Str);
		return Result;
	}
	else return Str;
}
//---------------------------------------------------------------------
