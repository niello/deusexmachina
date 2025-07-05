#include "FileSystemNative.h"

#include <Data/Array.h>
#include <IO/PathUtils.h>
#include <System/OSFileSystem.h>

// Can be reimplemented with C++17 std filesystem library without platform-specific interface.
// Need to check performance under Windows.

namespace IO
{

CFileSystemNative::CFileSystemNative(DEM::Sys::IOSFileSystem* pHostFS, const char* pRootPath, bool ReadOnly)
	: pFS(pHostFS)
	, Root(pRootPath)
	, _ReadOnly(ReadOnly)
{
}
//---------------------------------------------------------------------

bool CFileSystemNative::Init()
{
	if (!pFS) FAIL;

	// Empty root mounts all the host FS, which always exists logically
	if (!Root.IsValid()) OK;

	PathUtils::EnsurePathHasEndingDirSeparator(Root);

	if (pFS->DirectoryExists(Root.CStr())) OK;

	return pFS->CreateDirectory(Root.CStr());
}
//---------------------------------------------------------------------

bool CFileSystemNative::FileExists(const char* pPath)
{
	return pFS->FileExists((Root + pPath).CStr());
}
//---------------------------------------------------------------------

bool CFileSystemNative::IsFileReadOnly(const char* pPath)
{
	return _ReadOnly || pFS->IsFileReadOnly((Root + pPath).CStr());
}
//---------------------------------------------------------------------

bool CFileSystemNative::SetFileReadOnly(const char* pPath, bool ReadOnly)
{
	return (!_ReadOnly || ReadOnly) && pFS->SetFileReadOnly((Root + pPath).CStr(), ReadOnly);
}
//---------------------------------------------------------------------

bool CFileSystemNative::DeleteFile(const char* pPath)
{
	return !_ReadOnly && pFS->DeleteFile((Root + pPath).CStr());
}
//---------------------------------------------------------------------

bool CFileSystemNative::CopyFile(const char* pSrcPath, const char* pDestPath)
{
	return !_ReadOnly && pFS->CopyFile((Root + pSrcPath).CStr(), (Root + pDestPath).CStr());
}
//---------------------------------------------------------------------

bool CFileSystemNative::DirectoryExists(const char* pPath)
{
	return pFS->DirectoryExists((Root + pPath).CStr());
}
//---------------------------------------------------------------------

bool CFileSystemNative::CreateDirectory(const char* pPath)
{
	return !_ReadOnly && pFS->CreateDirectory((Root + pPath).CStr());
}
//---------------------------------------------------------------------

bool CFileSystemNative::DeleteDirectory(const char* pPath)
{
	return !_ReadOnly && pFS->DeleteDirectory((Root + pPath).CStr());
}
//---------------------------------------------------------------------

void* CFileSystemNative::OpenDirectory(const char* pPath, const char* pFilter, CString& OutName, EFSEntryType& OutType)
{
	return pFS->OpenDirectory((Root + pPath).CStr(), pFilter, OutName, OutType);
}
//---------------------------------------------------------------------

void CFileSystemNative::CloseDirectory(void* hDir)
{
	pFS->CloseDirectory(hDir);
}
//---------------------------------------------------------------------

bool CFileSystemNative::NextDirectoryEntry(void* hDir, CString& OutName, EFSEntryType& OutType)
{
	return pFS->NextDirectoryEntry(hDir, OutName, OutType);
}
//---------------------------------------------------------------------

void* CFileSystemNative::OpenFile(const char* pPath, EStreamAccessMode Mode, EStreamAccessPattern Pattern)
{
	if (_ReadOnly && (Mode & (EStreamAccessMode::SAM_WRITE | EStreamAccessMode::SAM_APPEND))) return nullptr;
	return pFS->OpenFile((Root + pPath).CStr(), Mode, Pattern);
}
//---------------------------------------------------------------------

void CFileSystemNative::CloseFile(void* hFile)
{
	pFS->CloseFile(hFile);
}
//---------------------------------------------------------------------

UPTR CFileSystemNative::Read(void* hFile, void* pData, UPTR Size)
{
	return pFS->Read(hFile, pData, Size);
}
//---------------------------------------------------------------------

UPTR CFileSystemNative::Write(void* hFile, const void* pData, UPTR Size)
{
	return _ReadOnly ? 0 : pFS->Write(hFile, pData, Size);
}
//---------------------------------------------------------------------

bool CFileSystemNative::Seek(void* hFile, I64 Offset, ESeekOrigin Origin)
{
	return pFS->Seek(hFile, Offset, Origin);
}
//---------------------------------------------------------------------

void CFileSystemNative::Flush(void* hFile)
{
	if (!_ReadOnly) pFS->Flush(hFile);
}
//---------------------------------------------------------------------

bool CFileSystemNative::IsEOF(void* hFile) const
{
	return pFS->IsEOF(hFile);
}
//---------------------------------------------------------------------

U64 CFileSystemNative::GetFileSize(void* hFile) const
{
	return pFS->GetFileSize(hFile);
}
//---------------------------------------------------------------------

U64 CFileSystemNative::GetFileWriteTime(void* hFile) const
{
	return pFS->GetFileWriteTime(hFile);
}
//---------------------------------------------------------------------

U64 CFileSystemNative::Tell(void* hFile) const
{
	return pFS->Tell(hFile);
}
//---------------------------------------------------------------------

bool CFileSystemNative::Truncate(void* hFile)
{
	return pFS->Truncate(hFile);
}
//---------------------------------------------------------------------

}
