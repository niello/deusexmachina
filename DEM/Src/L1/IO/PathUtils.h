#pragma once
#ifndef __DEM_L1_IO_PATH_UTILS_H__
#define __DEM_L1_IO_PATH_UTILS_H__

#include <StdDEM.h>

// Path and URI utility functions

namespace IO { namespace PathUtils
{

// Get a pointer to the last directory separator.
inline const char* GetLastDirSeparator(const char* pPath)
{
	const char* pLastSlash = strrchr(pPath, '/');
	if (!pLastSlash) pLastSlash = strrchr(pPath, '\\');
	if (!pLastSlash) pLastSlash = strrchr(pPath, ':');
	return pLastSlash;
}
//---------------------------------------------------------------------

inline int GetLastDirSeparatorIndex(const char* pPath)
{
	const char* pSep = GetLastDirSeparator(pPath);
	return pSep ? pSep - pPath : -1;
}
//---------------------------------------------------------------------

// Returns pointer to extension (without the dot), or NULL
inline const char* GetExtension(const char* pPath)
{
	const char* pLastDirSep = GetLastDirSeparator(pPath);
	const char* pStr = strrchr(pLastDirSep ? pLastDirSep + 1 : pPath, '.');
	return (pStr && *(++pStr)) ? pStr : NULL;
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

inline CString ExtractFileName()
{
	char* pLastDirSep = GetLastDirSeparator();
	CString Path = pLastDirSep ? pLastDirSep + 1 : CStr();
	return Path;
}
//---------------------------------------------------------------------

inline CString ExtractFileNameWithoutExtension()
{
	char* pLastDirSep = GetLastDirSeparator();
	CString Path = pLastDirSep ? pLastDirSep + 1 : CStr();
	return Path;
}
//---------------------------------------------------------------------

// Return a CString object containing the last directory of the path, i.e. a category
inline CString ExtractLastDirName()
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
inline CString ExtractDirName()
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
inline CString ExtractToLastSlash()
{
	CString Path(*this);
	char* pLastDirSep = Path.GetLastDirSeparator();
	if (pLastDirSep) pLastDirSep[1] = 0;
	else Path = "";
	return Path;
}
//---------------------------------------------------------------------

inline CString StripExtension()
{
	char* ext = (char*)GetExtension();
	if (ext) ext[-1] = 0;
	SetLength(strlen(CStr()));
}
//---------------------------------------------------------------------

} }

#endif
