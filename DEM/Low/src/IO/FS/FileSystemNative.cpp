#include "FileSystemNative.h"

#include <Data/Array.h>
#include <IO/PathUtils.h>
#include <System/OSFileSystem.h>

// Can be reimplemented with C++17 std filesystem library without platform-specific interface.
// Need to check performance under Windows.

namespace IO
{

CFileSystemNative::CFileSystemNative(DEM::Sys::IOSFileSystem* pHostFS, const char* pRootPath)
	: pFS(pHostFS)
	, Root(pRootPath)
{
	if (Root.IsValid()) PathUtils::EnsurePathHasEndingDirSeparator(Root);
}
//---------------------------------------------------------------------

bool CFileSystemNative::Init()
{
	if (!pFS) FAIL;

	// Empty root mounts all the host FS, which always exists logically
	return Root.IsValid() ? pFS->DirectoryExists(Root.CStr()) : true;
}
//---------------------------------------------------------------------

bool CFileSystemNative::FileExists(const char* pPath)
{
	return pFS->FileExists(Root + pPath);
}
//---------------------------------------------------------------------

bool CFileSystemNative::IsFileReadOnly(const char* pPath)
{
	return pFS->IsFileReadOnly(Root + pPath);
}
//---------------------------------------------------------------------

bool CFileSystemNative::SetFileReadOnly(const char* pPath, bool ReadOnly)
{
	return pFS->SetFileReadOnly(Root + pPath, ReadOnly);
}
//---------------------------------------------------------------------

bool CFileSystemNative::DeleteFile(const char* pPath)
{
	return pFS->DeleteFile(Root + pPath);
}
//---------------------------------------------------------------------

bool CFileSystemNative::CopyFile(const char* pSrcPath, const char* pDestPath)
{
	return pFS->CopyFile(Root + pSrcPath, Root + pDestPath);
}
//---------------------------------------------------------------------

bool CFileSystemNative::DirectoryExists(const char* pPath)
{
	return pFS->DirectoryExists(Root + pPath);
}
//---------------------------------------------------------------------

bool CFileSystemNative::CreateDirectory(const char* pPath)
{
	return pFS->CreateDirectory(Root + pPath);
}
//---------------------------------------------------------------------

bool CFileSystemNative::DeleteDirectory(const char* pPath)
{
	return pFS->DeleteDirectory(Root + pPath);
}
//---------------------------------------------------------------------

void* CFileSystemNative::OpenDirectory(const char* pPath, const char* pFilter, CString& OutName, EFSEntryType& OutType)
{
	return pFS->OpenDirectory(Root + pPath, pFilter, OutName, OutType);
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
	return pFS->OpenFile(Root + pPath, Mode, Pattern);
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
	return pFS->Write(hFile, pData, Size);
}
//---------------------------------------------------------------------

bool CFileSystemNative::Seek(void* hFile, I64 Offset, ESeekOrigin Origin)
{
	return pFS->Seek(hFile, Offset, Origin);
}
//---------------------------------------------------------------------

void CFileSystemNative::Flush(void* hFile)
{
	pFS->Flush(hFile);
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

}