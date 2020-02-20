#if DEM_PLATFORM_WIN32
#pragma once
#include <System/OSFileSystem.h>

// Win32 platform-dependent file system access

namespace DEM { namespace Sys
{

class COSFileSystemWin32 : public IOSFileSystem
{
public:

	virtual bool	IsValidFileName(const char* pName) const override;
	virtual bool	FileExists(const char* pPath) override;
	virtual bool	IsFileReadOnly(const char* pPath) override;
	virtual bool	SetFileReadOnly(const char* pPath, bool ReadOnly) override;
	virtual bool	DeleteFile(const char* pPath) override;
	virtual bool	CopyFile(const char* pSrcPath, const char* pDestPath) override;
	virtual bool	DirectoryExists(const char* pPath) override;
	virtual bool	CreateDirectory(const char* pPath) override;
	virtual bool	DeleteDirectory(const char* pPath) override;

	virtual void*	OpenDirectory(const char* pPath, const char* pFilter, CString& OutName, IO::EFSEntryType& OutType) override;
	virtual void	CloseDirectory(void* hDir) override;
	virtual bool	NextDirectoryEntry(void* hDir, CString& OutName, IO::EFSEntryType& OutType) override;

	virtual void*	OpenFile(const char* pPath, IO::EStreamAccessMode Mode, IO::EStreamAccessPattern Pattern = IO::SAP_DEFAULT) override;
	virtual void	CloseFile(void* hFile) override;
	virtual UPTR	Read(void* hFile, void* pData, UPTR Size) override;
	virtual UPTR	Write(void* hFile, const void* pData, UPTR Size) override;
	virtual U64		GetFileSize(void* hFile) const override;
	virtual U64		GetFileWriteTime(void* hFile) const override;
	virtual bool	Seek(void* hFile, I64 Offset, IO::ESeekOrigin Origin) override;
	virtual U64		Tell(void* hFile) const override;
	virtual bool	Truncate(void* hFile) override;
	virtual void	Flush(void* hFile) override;
	virtual bool	IsEOF(void* hFile) const override;
};

}
};

#endif
