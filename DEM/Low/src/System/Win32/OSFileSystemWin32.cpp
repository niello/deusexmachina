#if DEM_PLATFORM_WIN32
#include "OSFileSystemWin32.h"
#include <IO/PathUtils.h>
#include <Data/Array.h>
#include <regex>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace DEM { namespace Sys
{

bool COSFileSystemWin32::IsValidFileName(const char* pName) const
{
	// This turned out to be a surprisingly hard task, so this method may
	// not work in some cases. But for most cases it must work well.
	std::string FileName(pName);
	std::smatch smatch;
	std::regex regex("^(?!(COM[0-9]|LPT[0-9]|CON|PRN|AUX|CLOCK\\$|NUL)$)[^\\/\\\\:*?\\\"<>|]{1,256}$");
	return std::regex_match(FileName, smatch, regex) && !smatch.empty();
}
//---------------------------------------------------------------------

bool COSFileSystemWin32::FileExists(const char* pPath)
{
	DWORD FileAttrs = ::GetFileAttributes(pPath);
	return FileAttrs != INVALID_FILE_ATTRIBUTES && !(FileAttrs & FILE_ATTRIBUTE_DIRECTORY);
}
//---------------------------------------------------------------------

bool COSFileSystemWin32::IsFileReadOnly(const char* pPath)
{
	DWORD FileAttrs = ::GetFileAttributes(pPath);
	return FileAttrs != INVALID_FILE_ATTRIBUTES && (FileAttrs & FILE_ATTRIBUTE_READONLY);
}
//---------------------------------------------------------------------

bool COSFileSystemWin32::SetFileReadOnly(const char* pPath, bool ReadOnly)
{
	DWORD FileAttrs = ::GetFileAttributes(pPath);
	if (FileAttrs == INVALID_FILE_ATTRIBUTES) FAIL;
	if ((FileAttrs & FILE_ATTRIBUTE_READONLY) == ReadOnly) OK;
	if (ReadOnly) FileAttrs |= FILE_ATTRIBUTE_READONLY;
	else FileAttrs &= ~FILE_ATTRIBUTE_READONLY;
	return ::SetFileAttributes(pPath, FileAttrs) != FALSE;
}
//---------------------------------------------------------------------

#undef DeleteFile
bool COSFileSystemWin32::DeleteFile(const char* pPath)
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
bool COSFileSystemWin32::CopyFile(const char* pSrcPath, const char* pDestPath)
{
#ifdef UNICODE
#define CopyFile  CopyFileW
#else
#define CopyFile  CopyFileA
#endif

	// Make the file writable if it exist and is read-only
	DWORD FileAttrs = ::GetFileAttributes(pDestPath);
	if (FileAttrs != INVALID_FILE_ATTRIBUTES)
	{
		if (FileAttrs & FILE_ATTRIBUTE_READONLY)
		{
			FileAttrs &= ~FILE_ATTRIBUTE_READONLY;
			if (::SetFileAttributes(pDestPath, FileAttrs) == FALSE) FAIL;
		}
	}

	return ::CopyFile(pSrcPath, pDestPath, FALSE) != 0;
}
//---------------------------------------------------------------------

bool COSFileSystemWin32::DirectoryExists(const char* pPath)
{
	DWORD FileAttrs = ::GetFileAttributes(pPath);
	return FileAttrs != INVALID_FILE_ATTRIBUTES && (FileAttrs & FILE_ATTRIBUTE_DIRECTORY);
}
//---------------------------------------------------------------------

#undef CreateDirectory
bool COSFileSystemWin32::CreateDirectory(const char* pPath)
{
#ifdef UNICODE
#define CreateDirectory  CreateDirectoryW
#else
#define CreateDirectory  CreateDirectoryA
#endif

	CString AbsPath(pPath);

	CArray<CString> DirStack;
	while (!DirectoryExists(AbsPath.CStr()))
	{
		AbsPath.Trim(" \r\n\t\\/", false);
		int LastSepIdx = PathUtils::GetLastDirSeparatorIndex(AbsPath.CStr());
		if (LastSepIdx >= 0)
		{
			DirStack.Add(AbsPath.SubString(LastSepIdx + 1, AbsPath.GetLength() - (LastSepIdx + 1)));
			AbsPath = AbsPath.SubString(0, LastSepIdx);
		}
		else
		{
			if (!CreateDirectory(AbsPath.CStr(), nullptr)) FAIL;
			break;
		}
	}

	while (DirStack.GetCount())
	{
		AbsPath += "/" + DirStack.Back();
		DirStack.RemoveAt(DirStack.GetCount() - 1);
		if (!CreateDirectory(AbsPath.CStr(), nullptr)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

bool COSFileSystemWin32::DeleteDirectory(const char* pPath)
{
	if (::RemoveDirectory(pPath) != FALSE) OK;

	// Failed to delete, so clear directory contents before

	// In the ANSI version of this function, the name is limited to MAX_PATH characters.
	// To extend this limit to 32,767 widecharacters, call the Unicode version of the
	// function and prepend "\\?\" to the path. For more information, see Naming a File.
	// (c)) MSDN
	char pPathBuf[MAX_PATH];
	sprintf_s(pPathBuf, MAX_PATH - 1, "%s\\*", pPath);

	WIN32_FIND_DATA FindData;
	HANDLE hRec = ::FindFirstFile(pPathBuf, &FindData);

	if (hRec == INVALID_HANDLE_VALUE) FAIL;

	do
	{
		if (!strcmp(FindData.cFileName, "..") || !strcmp(FindData.cFileName, ".")) continue;

		CString Name(pPath);
		Name += "/";
		Name += FindData.cFileName;
		if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (!DeleteDirectory(Name.CStr())) FAIL;
		}
		else if (::DeleteFile(Name.CStr()) == FALSE) FAIL;
	}
	while (::FindNextFile(hRec, &FindData) != FALSE);

	if (::GetLastError() != ERROR_NO_MORE_FILES) FAIL;
	::FindClose(hRec);

	return ::RemoveDirectory(pPath) != FALSE;
}
//---------------------------------------------------------------------

void* COSFileSystemWin32::OpenDirectory(const char* pPath, const char* pFilter, CString& OutName, IO::EFSEntryType& OutType)
{
	DWORD FileAttrs = ::GetFileAttributes(pPath);
	if (FileAttrs == INVALID_FILE_ATTRIBUTES || !(FileAttrs & FILE_ATTRIBUTE_DIRECTORY)) return nullptr;

	const char* pActualFilter = (pFilter && *pFilter) ? pFilter : "/*.*"; //???or "/*" ?
	UPTR FilterLength = strlen(pActualFilter);
	CString SearchString(pPath, strlen(pPath), FilterLength);
	SearchString.Add(pActualFilter, FilterLength);

	WIN32_FIND_DATA FindData;
	HANDLE hDir = ::FindFirstFile(SearchString.CStr(), &FindData);

	if (hDir == INVALID_HANDLE_VALUE)
	{
		OutName.Clear();
		OutType = IO::FSE_NONE;
		return nullptr; //???return bool success instead? mb nullptr handle is valid?
	}

	while (!strcmp(FindData.cFileName, "..") || !strcmp(FindData.cFileName, "."))
		if (!::FindNextFile(hDir, &FindData))
		{
			OutName.Clear();
			OutType = IO::FSE_NONE;
			return hDir;
		}

	OutName = FindData.cFileName;
	OutType = (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? IO::FSE_DIR : IO::FSE_FILE;

	return hDir;
}
//---------------------------------------------------------------------

void COSFileSystemWin32::CloseDirectory(void* hDir)
{
	n_assert(hDir);
	::FindClose(hDir);
}
//---------------------------------------------------------------------

bool COSFileSystemWin32::NextDirectoryEntry(void* hDir, CString& OutName, IO::EFSEntryType& OutType)
{
	n_assert(hDir);
	WIN32_FIND_DATA FindData;
	if (::FindNextFile(hDir, &FindData) != 0)
	{
		OutName = FindData.cFileName;
		OutType = (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? IO::FSE_DIR : IO::FSE_FILE;
		OK;
	}
	OutName.Clear();
	OutType = IO::FSE_NONE;
	FAIL;
}
//---------------------------------------------------------------------

void* COSFileSystemWin32::OpenFile(const char* pPath, IO::EStreamAccessMode Mode, IO::EStreamAccessPattern Pattern)
{
	if (!pPath || !*pPath) return nullptr;

	DWORD Access = 0;
	if ((Mode & IO::SAM_READ) || (Mode & IO::SAM_APPEND)) Access |= GENERIC_READ;
	if ((Mode & IO::SAM_WRITE) || (Mode & IO::SAM_APPEND)) Access |= GENERIC_WRITE;

	//!!!MSDN recommends MMF to be opened exclusively!
	//???mb set ShareMode = 0 for release builds? is there any PERF gain?
	DWORD ShareMode = FILE_SHARE_READ;
	if (!(Mode & IO::SAM_WRITE) || (Mode & IO::SAM_READ)) ShareMode |= FILE_SHARE_WRITE;

	DWORD Disposition = 0;
	switch (Mode)
	{
		case IO::SAM_READ:		Disposition = OPEN_EXISTING; break;
		case IO::SAM_WRITE:		Disposition = CREATE_ALWAYS; break;
		case IO::SAM_READWRITE:
		case IO::SAM_APPEND:	Disposition = OPEN_ALWAYS; break;
	}

	HANDLE hFile = ::CreateFile(pPath,
		Access,
		ShareMode,
		0,
		Disposition,
		(Pattern == IO::SAP_RANDOM) ? FILE_FLAG_RANDOM_ACCESS : FILE_FLAG_SEQUENTIAL_SCAN,
		nullptr);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		if (Mode == IO::SAM_APPEND) ::SetFilePointer(hFile, 0, nullptr, FILE_END);
		return hFile;
	}

	return nullptr; //???return bool success instead? mb nullptr handle is valid?
}
//---------------------------------------------------------------------

void COSFileSystemWin32::CloseFile(void* hFile)
{
	n_assert(hFile);
	::CloseHandle(hFile);
}
//---------------------------------------------------------------------

UPTR COSFileSystemWin32::Read(void* hFile, void* pData, UPTR Size)
{
	n_assert(hFile && pData && Size > 0 && Size <= ULONG_MAX);
	DWORD BytesRead;
	BOOL Result = ::ReadFile(hFile, pData, (DWORD)Size, &BytesRead, nullptr);
	return (Result == TRUE) ? BytesRead : 0;
}
//---------------------------------------------------------------------

UPTR COSFileSystemWin32::Write(void* hFile, const void* pData, UPTR Size)
{
	n_assert(hFile && pData && Size > 0 && Size <= ULONG_MAX);
	DWORD BytesWritten;
	BOOL Result = ::WriteFile(hFile, pData, (DWORD)Size, &BytesWritten, nullptr);
	return (Result == TRUE) ? BytesWritten : 0;
}
//---------------------------------------------------------------------

bool COSFileSystemWin32::Seek(void* hFile, I64 Offset, IO::ESeekOrigin Origin)
{
	n_assert(hFile);
	DWORD SeekOrigin;
	switch (Origin)
	{
		case IO::Seek_Current:	SeekOrigin = FILE_CURRENT; break;
		case IO::Seek_End:		SeekOrigin = FILE_END; Offset = -Offset; break;
		default:				SeekOrigin = FILE_BEGIN; break;
	}
	LARGE_INTEGER LIOffset;
	LIOffset.QuadPart = Offset;
	return ::SetFilePointerEx(hFile, LIOffset, nullptr, SeekOrigin) != 0;
}
//---------------------------------------------------------------------

void COSFileSystemWin32::Flush(void* hFile)
{
	n_assert(hFile);
	::FlushFileBuffers(hFile);
}
//---------------------------------------------------------------------

bool COSFileSystemWin32::IsEOF(void* hFile) const
{
	n_assert(hFile);
	LARGE_INTEGER Pos;
	LARGE_INTEGER Size;
	if (!::SetFilePointerEx(hFile, { 0 }, &Pos, FILE_CURRENT)) OK;
	if (!::GetFileSizeEx(hFile, &Size)) OK;
	return Pos.QuadPart >= Size.QuadPart;
}
//---------------------------------------------------------------------

U64 COSFileSystemWin32::GetFileSize(void* hFile) const
{
	n_assert_dbg(hFile);
	LARGE_INTEGER Size;
	if (!::GetFileSizeEx(hFile, &Size)) return 0;
	return (U64)Size.QuadPart;
}
//---------------------------------------------------------------------

U64 COSFileSystemWin32::GetFileWriteTime(void* hFile) const
{
	FILETIME WriteTime;
	if (!::GetFileTime(hFile, nullptr, nullptr, &WriteTime)) return 0;

	ULARGE_INTEGER UL;
	UL.LowPart = WriteTime.dwLowDateTime;
	UL.HighPart = WriteTime.dwHighDateTime;
	UL.QuadPart /= 10000000; // To seconds
	return (U64)(UL.QuadPart - 11644473600ULL); // Seconds from Win file time base to unix epoch base
}
//---------------------------------------------------------------------

U64 COSFileSystemWin32::Tell(void* hFile) const
{
	n_assert(hFile);
	LARGE_INTEGER Pos;
	if (!::SetFilePointerEx(hFile, { 0 }, &Pos, FILE_CURRENT)) return 0;
	return (U64)Pos.QuadPart;
}
//---------------------------------------------------------------------

bool COSFileSystemWin32::Truncate(void* hFile)
{
	n_assert(hFile);
	return ::SetEndOfFile(hFile) != 0;
}
//---------------------------------------------------------------------

}};

#endif
