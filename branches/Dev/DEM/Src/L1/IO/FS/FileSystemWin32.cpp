#include "FileSystemWin32.h"

#include <Data/Array.h>
#include <IO/PathUtils.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>

namespace IO
{
__ImplementSingleton(CFileSystemWin32);

CFileSystemWin32::CFileSystemWin32()
{
	__ConstructSingleton;
}
//---------------------------------------------------------------------

CFileSystemWin32::~CFileSystemWin32()
{
	__DestructSingleton;
}
//---------------------------------------------------------------------

bool CFileSystemWin32::FileExists(const char* pPath)
{
	DWORD FileAttrs = GetFileAttributes(pPath);
	return FileAttrs != INVALID_FILE_ATTRIBUTES && !(FileAttrs & FILE_ATTRIBUTE_DIRECTORY);
}
//---------------------------------------------------------------------

bool CFileSystemWin32::IsFileReadOnly(const char* pPath)
{
	DWORD FileAttrs = GetFileAttributes(pPath);
	return FileAttrs != INVALID_FILE_ATTRIBUTES && (FileAttrs & FILE_ATTRIBUTE_READONLY);
}
//---------------------------------------------------------------------

bool CFileSystemWin32::SetFileReadOnly(const char* pPath, bool ReadOnly)
{
	DWORD FileAttrs = GetFileAttributes(pPath);
	if (FileAttrs == INVALID_FILE_ATTRIBUTES) FAIL;
	if (ReadOnly) FileAttrs |= FILE_ATTRIBUTE_READONLY;
	else FileAttrs &= ~FILE_ATTRIBUTE_READONLY;
	return SetFileAttributes(pPath, FileAttrs) != FALSE;
}
//---------------------------------------------------------------------

#undef DeleteFile
bool CFileSystemWin32::DeleteFile(const char* pPath)
{
#ifdef UNICODE
#define DeleteFile  DeleteFileW
#else
#define DeleteFile  DeleteFileA
#endif
	return ::DeleteFile(pPath) != 0;
}
//---------------------------------------------------------------------

#undef CopyFile
bool CFileSystemWin32::CopyFile(const char* pSrcPath, const char* pDestPath)
{
#ifdef UNICODE
#define CopyFile  CopyFileW
#else
#define CopyFile  CopyFileA
#endif
	if (IsFileReadOnly(pDestPath) && !SetFileReadOnly(pDestPath, false)) FAIL;
	return ::CopyFile(pSrcPath, pDestPath, FALSE) != 0;
}
//---------------------------------------------------------------------

bool CFileSystemWin32::DirectoryExists(const char* pPath)
{
	DWORD FileAttrs = GetFileAttributes(pPath);
	return FileAttrs != INVALID_FILE_ATTRIBUTES && (FileAttrs & FILE_ATTRIBUTE_DIRECTORY);
}
//---------------------------------------------------------------------

#undef CreateDirectory
bool CFileSystemWin32::CreateDirectory(const char* pPath)
{
#ifdef UNICODE
#define CreateDirectory  CreateDirectoryW
#else
#define CreateDirectory  CreateDirectoryA
#endif

	CString AbsPath(pPath);

	CArray<CString> DirStack;
	while (!DirectoryExists(AbsPath))
	{
		AbsPath.Trim(" \r\n\t\\/", false);
		int LastSepIdx = PathUtils::GetLastDirSeparatorIndex(AbsPath);
		if (LastSepIdx >= 0)
		{
			DirStack.Add(AbsPath.SubString(LastSepIdx + 1, AbsPath.GetLength() - (LastSepIdx + 1)));
			AbsPath = AbsPath.SubString(0, LastSepIdx);
		}
		else
		{
			if (!CreateDirectory(AbsPath.CStr(), NULL)) FAIL;
			break;
		}
	}

	while (DirStack.GetCount())
	{
		AbsPath += "/" + DirStack.Back();
		DirStack.RemoveAt(DirStack.GetCount() - 1);
		if (!CreateDirectory(AbsPath.CStr(), NULL)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

bool CFileSystemWin32::DeleteDirectory(const char* pPath)
{
	if (RemoveDirectory(pPath) != FALSE) OK;

	// Failed to delete, so clear directory contents before

	// In the ANSI version of this function, the name is limited to MAX_PATH characters.
	// To extend this limit to 32,767 widecharacters, call the Unicode version of the
	// function and prepend "\\?\" to the path. For more information, see Naming a File.
	// (c)) MSDN
	char pPathBuf[MAX_PATH];
	sprintf_s(pPathBuf, MAX_PATH - 1, "%s\\*", pPath);

	WIN32_FIND_DATA FindData;
	HANDLE hRec = FindFirstFile(pPathBuf, &FindData);

	if (hRec == INVALID_HANDLE_VALUE) FAIL;

	do
	{
		if (!strcmp(FindData.cFileName, "..") || !strcmp(FindData.cFileName, ".")) continue;

		CString Name(pPath);
		Name += "/";
		Name += FindData.cFileName;
		if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (!DeleteDirectory(Name)) FAIL;
		}
		else if (::DeleteFile(Name.CStr()) == FALSE) FAIL;
	}
	while (FindNextFile(hRec, &FindData) != FALSE);

	if (GetLastError() != ERROR_NO_MORE_FILES) FAIL;
	FindClose(hRec);

	return RemoveDirectory(pPath) != FALSE;
}
//---------------------------------------------------------------------

bool CFileSystemWin32::GetSystemFolderPath(ESystemFolder Code, CString& OutPath)
{
    char pRawPath[DEM_MAX_PATH];
	if (Code == SF_TEMP)
	{
		if (!GetTempPath(sizeof(pRawPath), pRawPath)) FAIL;
		OutPath = pRawPath;
		OutPath.Replace('\\', '/');
	}
	else if (Code == SF_HOME || Code == SF_BIN)
	{
		if (!GetModuleFileName(NULL, pRawPath, sizeof(pRawPath))) FAIL;
		CString PathToExe(pRawPath);
		PathToExe.Replace('\\', '/');
		OutPath = PathUtils::ExtractDirName(PathToExe);
	}
	else
	{
		int CSIDL;
		switch (Code)
		{
			case SF_USER:		CSIDL = CSIDL_PERSONAL | CSIDL_FLAG_CREATE; break;
			case SF_APP_DATA:	CSIDL = CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE; break;
			case SF_PROGRAMS:	CSIDL = CSIDL_PROGRAM_FILES; break;
			default:			FAIL;
		}

		if (FAILED(SHGetFolderPath(0, CSIDL, NULL, 0, pRawPath))) FAIL;
		OutPath = pRawPath;
		OutPath.Replace('\\', '/');
	}

	OK;
}
//---------------------------------------------------------------------

void* CFileSystemWin32::OpenDirectory(const char* pPath, const char* pFilter, CString& OutName, EFSEntryType& OutType)
{
	DWORD FileAttrs = GetFileAttributes(pPath);
	if (FileAttrs == INVALID_FILE_ATTRIBUTES || !(FileAttrs & FILE_ATTRIBUTE_DIRECTORY)) return NULL;

	const char* pActualFilter = (pFilter && *pFilter) ? pFilter : "/*.*"; //???or "/*" ?
	DWORD FilterLength = strlen(pActualFilter);
	CString SearchString(pPath, strlen(pPath), FilterLength);
	SearchString.Add(pActualFilter, FilterLength);

	WIN32_FIND_DATA FindData;
	HANDLE hDir = FindFirstFile(SearchString.CStr(), &FindData);

	if (hDir == INVALID_HANDLE_VALUE)
	{
		OutName.Clear();
		OutType = FSE_NONE;
		return NULL; //???return bool success instead? mb NULL handle is valid?
	}

	while (!strcmp(FindData.cFileName, "..") || !strcmp(FindData.cFileName, "."))
		if (!FindNextFile(hDir, &FindData))
		{
			OutName.Clear();
			OutType = FSE_NONE;
			return hDir;
		}

	OutName = FindData.cFileName;
	OutType = (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? FSE_DIR : FSE_FILE;

	return hDir;
}
//---------------------------------------------------------------------

void CFileSystemWin32::CloseDirectory(void* hDir)
{
	n_assert(hDir);
	FindClose(hDir);
}
//---------------------------------------------------------------------

bool CFileSystemWin32::NextDirectoryEntry(void* hDir, CString& OutName, EFSEntryType& OutType)
{
	n_assert(hDir);
	WIN32_FIND_DATA FindData;
	if (FindNextFile(hDir, &FindData) != 0)
	{
		OutName = FindData.cFileName;
		OutType = (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? FSE_DIR : FSE_FILE;
		OK;
	}
	OutName.Clear();
	OutType = FSE_NONE;
	FAIL;
}
//---------------------------------------------------------------------

void* CFileSystemWin32::OpenFile(const char* pPath, EStreamAccessMode Mode, EStreamAccessPattern Pattern)
{
	if (!pPath && !*pPath) return NULL;

	DWORD Access = 0;
	if ((Mode & SAM_READ) || (Mode & SAM_APPEND)) Access |= GENERIC_READ;
	if ((Mode & SAM_WRITE) || (Mode & SAM_APPEND)) Access |= GENERIC_WRITE;

	//!!!MSDN recommends MMF to be opened exclusively!
	//???mb set ShareMode = 0 for release builds? is there any PERF gain?
	DWORD ShareMode = FILE_SHARE_READ;
	if (!(Mode & SAM_WRITE) || (Mode & SAM_READ)) ShareMode |= FILE_SHARE_WRITE;

	DWORD Disposition = 0;
	switch (Mode)
	{
		case SAM_READ:		Disposition = OPEN_EXISTING; break;
		case SAM_WRITE:		Disposition = CREATE_ALWAYS; break;
		case SAM_READWRITE:
		case SAM_APPEND:	Disposition = OPEN_ALWAYS; break;
	}

	HANDLE hFile = CreateFile(	pPath,
								Access,
								ShareMode,
								0,
								Disposition,
								(Pattern == SAP_RANDOM) ? FILE_FLAG_RANDOM_ACCESS : FILE_FLAG_SEQUENTIAL_SCAN,
								NULL);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		if (Mode == SAM_APPEND) SetFilePointer(hFile, 0, NULL, FILE_END);
		return hFile;
	}

	return NULL; //???return bool success instead? mb NULL handle is valid?
}
//---------------------------------------------------------------------

void CFileSystemWin32::CloseFile(void* hFile)
{
	n_assert(hFile);
	CloseHandle(hFile);
}
//---------------------------------------------------------------------

DWORD CFileSystemWin32::Read(void* hFile, void* pData, DWORD Size)
{
	n_assert(hFile && pData && Size > 0);
	DWORD BytesRead;
	BOOL Result = ReadFile(hFile, pData, Size, &BytesRead, NULL);
	return (Result == TRUE) ? BytesRead : 0;
}
//---------------------------------------------------------------------

DWORD CFileSystemWin32::Write(void* hFile, const void* pData, DWORD Size)
{
	n_assert(hFile && pData && Size > 0);
	DWORD BytesWritten;
	BOOL Result = WriteFile(hFile, pData, Size, &BytesWritten, NULL);
	return (Result == TRUE) ? BytesWritten : 0;
}
//---------------------------------------------------------------------

bool CFileSystemWin32::Seek(void* hFile, int Offset, ESeekOrigin Origin)
{
	n_assert(hFile);
	DWORD SeekOrigin;
	switch (Origin)
	{
		case Seek_Current:	SeekOrigin = FILE_CURRENT; break;
		case Seek_End:		SeekOrigin = FILE_END; break;
		default:			SeekOrigin = FILE_BEGIN; break;
	}
	return SetFilePointer(hFile, Offset, NULL, SeekOrigin) != INVALID_SET_FILE_POINTER;
}
//---------------------------------------------------------------------

void CFileSystemWin32::Flush(void* hFile)
{
	n_assert(hFile);
	FlushFileBuffers(hFile);
}
//---------------------------------------------------------------------

bool CFileSystemWin32::IsEOF(void* hFile) const
{
	n_assert(hFile);
	return SetFilePointer(hFile, 0, NULL, FILE_CURRENT) >= ::GetFileSize(hFile, NULL);
}
//---------------------------------------------------------------------

DWORD CFileSystemWin32::GetFileSize(void* hFile) const
{
	n_assert(hFile);
	return ::GetFileSize(hFile, NULL);
}
//---------------------------------------------------------------------

DWORD CFileSystemWin32::Tell(void* hFile) const
{
	n_assert(hFile);
	return SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
}
//---------------------------------------------------------------------

}