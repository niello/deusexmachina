#pragma once
#include <StdDEM.h>

// Path and URI utility functions

namespace PathUtils
{

// Returns the same path with '.' (this folder) and '..' (parent folder) collapsed where possible.
// E.g. CollapseDots("One/Two/Three/../../Four") will return "One/Four"
// All '\' slashes in a path must be converted to '/' before calling this function.
std::string CollapseDots(const char* pPath, UPTR PathLength = 0);

// Get a pointer to the last directory separator.
//!!!use FindLastIndex(pCharSet)!
inline const char* GetLastDirSeparator(const char* pPath, UPTR PathLen = 0)
{
	if (!pPath) return nullptr;
	if (!PathLen) PathLen = strlen(pPath);

	const char* pCurr = pPath + PathLen - 1;
	while (pCurr >= pPath)
	{
		if (strchr("/\\:", *pCurr)) return pCurr;
		--pCurr;
	};
	return nullptr;
}
//---------------------------------------------------------------------

inline int GetLastDirSeparatorIndex(const char* pPath)
{
	const char* pSep = GetLastDirSeparator(pPath);
	return pSep ? pSep - pPath : -1;
}
//---------------------------------------------------------------------

inline void EnsurePathHasEndingDirSeparator(std::string& Path)
{
	UPTR PathLen = Path.size();
	if (PathLen && Path[PathLen - 1] != '/') Path += '/';
}
//---------------------------------------------------------------------

// Returns pointer to extension (without the dot), or empty string (not nullptr, for CRT comparison)
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

inline std::string ExtractFileName(const char* pPath)
{
	const char* pLastDirSep = GetLastDirSeparator(pPath);
	return std::string(pLastDirSep ? pLastDirSep + 1 : pPath);
}
//---------------------------------------------------------------------

inline std::string ExtractFileNameWithoutExtension(const char* pPath)
{
	const char* pLastDirSep = GetLastDirSeparator(pPath);
	const char* pStr = pLastDirSep ? pLastDirSep + 1 : pPath;
	const char* pExt = GetExtension(pStr);
	if (pExt) return std::string(pStr, pExt - pStr - 1); // - 1 to skip dot
	else return std::string(pStr);
}
//---------------------------------------------------------------------

// Return a std::string object containing the last directory of the path
inline std::string ExtractLastDirName(const char* pPath)
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

		const char* pSecondLastDirSep = nullptr;
		if (pLastDirSep)
		{
			pEnd = pLastDirSep;
			pSecondLastDirSep = GetLastDirSeparator(pPath, pEnd - pPath);
			if (pSecondLastDirSep) return std::string(pSecondLastDirSep + 1, pEnd - pSecondLastDirSep - 1);
		}
	}

	return {};
}
//---------------------------------------------------------------------

// Return a std::string object containing the part before the last directory separator.
// NOTE (floh): I left my fix in that returns the last slash (or colon), this was
// necessary to tell if a dirname is a normal directory or an assign.
inline std::string ExtractDirName(const char* pPath, UPTR PathLength = 0)
{
	if (!pPath) return {};

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

	return std::string(pPath, pEnd - pPath);
}
//---------------------------------------------------------------------

inline std::string ExtractDirName(const std::string& Path)
{
	return ExtractDirName(Path.c_str(), Path.size());
}
//---------------------------------------------------------------------

// Return a path pString object which contains of the complete path up to the last slash.
// Returns an empty pString if there is no slash in the path.
inline std::string ExtractToLastSlash(const char* pPath)
{
	const char* pLastDirSep = GetLastDirSeparator(pPath);
	if (pLastDirSep) return std::string(pPath, pLastDirSep - pPath);
	else return {};
}
//---------------------------------------------------------------------

// If relative path contains ':' it is considered absolute, and pCurrentPath is not used
inline std::string GetAbsolutePath(const char* pCurrentPath, const char* pRelativePath)
{
	if (!pCurrentPath || !pRelativePath || !*pCurrentPath || !*pRelativePath) return std::string(pRelativePath);

	if (strchr(pRelativePath, ':')) return CollapseDots(pRelativePath);

	std::string AbsPath(pCurrentPath);
	EnsurePathHasEndingDirSeparator(AbsPath);
	AbsPath += (*pRelativePath == '/') ? (pRelativePath + 1) : pRelativePath;
	return CollapseDots(AbsPath.c_str(), AbsPath.size());
}
//---------------------------------------------------------------------

}
