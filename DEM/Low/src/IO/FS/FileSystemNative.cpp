#include "FileSystemNative.h"

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
	if (Root.empty()) OK;

	PathUtils::EnsurePathHasEndingDirSeparator(Root);

	if (pFS->DirectoryExists(Root.c_str())) OK;

	return pFS->CreateDirectory(Root.c_str());
}
//---------------------------------------------------------------------

bool CFileSystemNative::FileExists(const char* pPath)
{
	return pFS->FileExists((Root + pPath).c_str());
}
//---------------------------------------------------------------------

bool CFileSystemNative::IsFileReadOnly(const char* pPath)
{
	return _ReadOnly || pFS->IsFileReadOnly((Root + pPath).c_str());
}
//---------------------------------------------------------------------

bool CFileSystemNative::SetFileReadOnly(const char* pPath, bool ReadOnly)
{
	return (!_ReadOnly || ReadOnly) && pFS->SetFileReadOnly((Root + pPath).c_str(), ReadOnly);
}
//---------------------------------------------------------------------

bool CFileSystemNative::DeleteFile(const char* pPath)
{
	return !_ReadOnly && pFS->DeleteFile((Root + pPath).c_str());
}
//---------------------------------------------------------------------

bool CFileSystemNative::CopyFile(const char* pSrcPath, const char* pDestPath)
{
	return !_ReadOnly && pFS->CopyFile((Root + pSrcPath).c_str(), (Root + pDestPath).c_str());
}
//---------------------------------------------------------------------

bool CFileSystemNative::DirectoryExists(const char* pPath)
{
	return pFS->DirectoryExists((Root + pPath).c_str());
}
//---------------------------------------------------------------------

bool CFileSystemNative::CreateDirectory(const char* pPath)
{
	return !_ReadOnly && pFS->CreateDirectory((Root + pPath).c_str());
}
//---------------------------------------------------------------------

bool CFileSystemNative::DeleteDirectory(const char* pPath)
{
	return !_ReadOnly && pFS->DeleteDirectory((Root + pPath).c_str());
}
//---------------------------------------------------------------------

void* CFileSystemNative::OpenDirectory(const char* pPath, const char* pFilter, std::string& OutName, EFSEntryType& OutType)
{
	return pFS->OpenDirectory((Root + pPath).c_str(), pFilter, OutName, OutType);
}
//---------------------------------------------------------------------

void CFileSystemNative::CloseDirectory(void* hDir)
{
	pFS->CloseDirectory(hDir);
}
//---------------------------------------------------------------------

bool CFileSystemNative::NextDirectoryEntry(void* hDir, std::string& OutName, EFSEntryType& OutType)
{
	return pFS->NextDirectoryEntry(hDir, OutName, OutType);
}
//---------------------------------------------------------------------

void* CFileSystemNative::OpenFile(const char* pPath, EStreamAccessMode Mode, EStreamAccessPattern Pattern)
{
	if (_ReadOnly && (Mode & (EStreamAccessMode::SAM_WRITE | EStreamAccessMode::SAM_APPEND))) return nullptr;
	return pFS->OpenFile((Root + pPath).c_str(), Mode, Pattern);
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
