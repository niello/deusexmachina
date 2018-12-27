#pragma once
#ifndef __DEM_L1_IO_PATH_UTILS_H__
#define __DEM_L1_IO_PATH_UTILS_H__

#include <Data/String.h>

// Path and URI utility functions

namespace PathUtils
{

// Returns the same path with '.' (this folder) and '..' (parent folder) collapsed where possible.
// E.g. CollapseDots("One/Two/Three/../../Four") will return "One/Four"
// All '\' slashes in a path must be converted to '/' before calling this function.
CString CollapseDots(const char* pPath, UPTR PathLength = 0);

// Get a pointer to the last directory separator.
//!!!use FindLastIndex(pCharSet)!
inline const char* GetLastDirSeparator(const char* pPath, UPTR PathLen = 0)
{
	if (!pPath) return NULL;
	if (!PathLen) PathLen = strlen(pPath);

	const char* pCurr = pPath + PathLen - 1;
	while (pCurr >= pPath)
	{
		if (strchr("/\\:", *pCurr)) return pCurr;
		--pCurr;
	};
	return NULL;
}
//---------------------------------------------------------------------

inline int GetLastDirSeparatorIndex(const char* pPath)
{
	const char* pSep = GetLastDirSeparator(pPath);
	return pSep ? pSep - pPath : -1;
}
//---------------------------------------------------------------------

inline void EnsurePathHasEndingDirSeparator(CString& Path)
{
	UPTR PathLen = Path.GetLength();
	if (PathLen && Path[PathLen - 1] != '/') Path += '/';
}
//---------------------------------------------------------------------

// Returns pointer to extension (without the dot), or empty string (not NULL, for CRT comparison)
inline const char* GetExtension(const char* pPath)
{
	const char* pLastDirSep = GetLastDirSeparator(pPath);
	const char* pStr = strrchr(pLastDirSep ? pLastDirSep + 1 : pPath, '.');
	return (pStr && *(++pStr)) ? pStr : "";
}
//---------------------------------------------------------------------

inline bool CheckExtension(const char* pPath, const char* pExtension)
{
	const char* pExtToCheck = pExtension;
	if (pExtToCheck && *pExtToCheck == '.') ++pExtToCheck;
	const char* pExt = GetExtension(pPath);
	return (pExt == pExtension) || !n_stricmp(pExtension, pExt);
}
//---------------------------------------------------------------------

inline CString ExtractFileName(const char* pPath)
{
	const char* pLastDirSep = GetLastDirSeparator(pPath);
	return CString(pLastDirSep ? pLastDirSep + 1 : pPath);
}
//---------------------------------------------------------------------

inline CString ExtractFileNameWithoutExtension(const char* pPath)
{
	const char* pLastDirSep = GetLastDirSeparator(pPath);
	const char* pStr = pLastDirSep ? pLastDirSep + 1 : pPath;
	const char* pExt = GetExtension(pStr);
	if (pExt) return CString(pStr, pExt - pStr - 1); // - 1 to skip dot
	else return CString(pStr);
}
//---------------------------------------------------------------------

// Return a CString object containing the last directory of the path
inline CString ExtractLastDirName(const char* pPath)
{
	UPTR PathLen = strlen(pPath);
	const char* pLastDirSep = GetLastDirSeparator(pPath, PathLen);

	if (pLastDirSep)
	{
		const char* pEnd = pPath + PathLen;
		if (!pLastDirSep[1])
		{
			--pEnd;
			pLastDirSep = GetLastDirSeparator(pPath, pEnd - pPath);
		}

		const char* pSecondLastDirSep = NULL;
		if (pLastDirSep)
		{
			pEnd = pLastDirSep;
			pSecondLastDirSep = GetLastDirSeparator(pPath, pEnd - pPath);
			if (pSecondLastDirSep) return CString(pSecondLastDirSep + 1, pEnd - pSecondLastDirSep - 1);
		}
	}

	return CString::Empty;
}
//---------------------------------------------------------------------

// Return a CString object containing the part before the last directory separator.
// NOTE (floh): I left my fix in that returns the last slash (or colon), this was
// necessary to tell if a dirname is a normal directory or an assign.
inline CString ExtractDirName(const char* pPath, UPTR PathLength = 0)
{
	if (!pPath) return CString::Empty;

	if (!PathLength) PathLength = strlen(pPath);
	const char* pLastDirSep = GetLastDirSeparator(pPath, PathLength);

	const char* pEnd = pPath + PathLength;
	if (pLastDirSep)
	{
		if (!pLastDirSep[1])
		{
			--pEnd;
			pLastDirSep = GetLastDirSeparator(pPath, pEnd - pPath);
		}
		if (pLastDirSep) pEnd = pLastDirSep + 1;
	}

	return CString(pPath, pEnd - pPath);
}
//---------------------------------------------------------------------

inline CString ExtractDirName(const CString& Path)
{
	return ExtractDirName(Path.CStr(), Path.GetLength());
}
//---------------------------------------------------------------------

// Return a path pString object which contains of the complete path up to the last slash.
// Returns an empty pString if there is no slash in the path.
inline CString ExtractToLastSlash(const char* pPath)
{
	const char* pLastDirSep = GetLastDirSeparator(pPath);
	if (pLastDirSep) return CString(pPath, pLastDirSep - pPath);
	else return CString::Empty;
}
//---------------------------------------------------------------------

// If relative path contains ':' it is considered absolute, and pCurrentPath is not used
inline CString GetAbsolutePath(const char* pCurrentPath, const char* pRelativePath)
{
	if (!pCurrentPath || !pRelativePath || !*pCurrentPath || !*pRelativePath) return CString(pRelativePath);

	if (strchr(pRelativePath, ':')) return CollapseDots(pRelativePath);

	CString AbsPath(pCurrentPath);
	EnsurePathHasEndingDirSeparator(AbsPath);
	AbsPath += (*pRelativePath == '/') ? (pRelativePath + 1) : pRelativePath;
	return CollapseDots(AbsPath.CStr(), AbsPath.GetLength());
}
//---------------------------------------------------------------------

}

#endif
