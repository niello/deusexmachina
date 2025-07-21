#pragma once
#include <IO/IOFwd.h>

// Platform-dependent native file system access interface. Implemented per-platform / OS.
// Cross-platform implementation may be added with C++17 std filesystem library.

namespace DEM::Sys
{

class IOSFileSystem
{
public:

	virtual bool	IsValidFileName(const char* pName) const = 0;
	virtual bool	FileExists(const char* pPath) = 0;
	virtual bool	IsFileReadOnly(const char* pPath) = 0;
	virtual bool	SetFileReadOnly(const char* pPath, bool ReadOnly) = 0;
	virtual bool	DeleteFile(const char* pPath) = 0;
	virtual bool	CopyFile(const char* pSrcPath, const char* pDestPath) = 0;
	virtual bool	DirectoryExists(const char* pPath) = 0;
	virtual bool	CreateDirectory(const char* pPath) = 0;
	virtual bool	DeleteDirectory(const char* pPath) = 0;

	virtual void*	OpenDirectory(const char* pPath, const char* pFilter, std::string& OutName, IO::EFSEntryType& OutType) = 0;
	virtual void	CloseDirectory(void* hDir) = 0;
	virtual bool	NextDirectoryEntry(void* hDir, std::string& OutName, IO::EFSEntryType& OutType) = 0;

	virtual void*	OpenFile(const char* pPath, IO::EStreamAccessMode Mode, IO::EStreamAccessPattern Pattern = IO::SAP_DEFAULT) = 0;
	virtual void	CloseFile(void* hFile) = 0;
	virtual UPTR	Read(void* hFile, void* pData, UPTR Size) = 0;
	virtual UPTR	Write(void* hFile, const void* pData, UPTR Size) = 0;
	virtual U64		GetFileSize(void* hFile) const = 0;
	virtual U64		GetFileWriteTime(void* hFile) const = 0;
	virtual bool	Seek(void* hFile, I64 Offset, IO::ESeekOrigin Origin) = 0;
	virtual U64		Tell(void* hFile) const = 0;
	virtual bool	Truncate(void* hFile) = 0;
	virtual void	Flush(void* hFile) = 0;
	virtual bool	IsEOF(void* hFile) const = 0;
};

}
