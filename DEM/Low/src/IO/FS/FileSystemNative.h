#pragma once
#include <IO/FileSystem.h>
#include <Data/String.h>

// Native OS file system. Use empty path to mount the whole host file system,
// though it is not recommended. Better mount only folders your app uses.

namespace DEM { namespace Sys
{
	class IOSFileSystem;
}}

namespace IO
{

class CFileSystemNative: public IFileSystem
{
protected:

	DEM::Sys::IOSFileSystem* pFS;
	CString Root;
	bool _ReadOnly;

public:

	//!!!platform interface required!
	CFileSystemNative(DEM::Sys::IOSFileSystem* pHostFS, const char* pRootPath, bool ReadOnly = false);

	virtual bool	Init() override;
	virtual bool	IsCaseSensitive() const override { FAIL; }
	virtual bool	IsReadOnly() const { return _ReadOnly; }
	virtual bool	ProvidesFileCursor() const { OK; }

	virtual bool	FileExists(const char* pPath);
	virtual bool	IsFileReadOnly(const char* pPath);
	virtual bool	SetFileReadOnly(const char* pPath, bool ReadOnly);
	virtual bool	DeleteFile(const char* pPath);
	virtual bool	CopyFile(const char* pSrcPath, const char* pDestPath);
	virtual bool	DirectoryExists(const char* pPath);
	virtual bool	CreateDirectory(const char* pPath);
	virtual bool	DeleteDirectory(const char* pPath);

	virtual void*	OpenDirectory(const char* pPath, const char* pFilter, CString& OutName, EFSEntryType& OutType);
	virtual void	CloseDirectory(void* hDir);
	virtual bool	NextDirectoryEntry(void* hDir, CString& OutName, EFSEntryType& OutType);

	virtual void*	OpenFile(const char* pPath, EStreamAccessMode Mode, EStreamAccessPattern Pattern = SAP_DEFAULT);
	virtual void	CloseFile(void* hFile);
	virtual UPTR	Read(void* hFile, void* pData, UPTR Size);
	virtual UPTR	Write(void* hFile, const void* pData, UPTR Size);
	virtual U64		GetFileSize(void* hFile) const;
	virtual U64		GetFileWriteTime(void* hFile) const;
	virtual bool	Seek(void* hFile, I64 Offset, ESeekOrigin Origin);
	virtual U64		Tell(void* hFile) const;
	virtual void	Flush(void* hFile);
	virtual bool	IsEOF(void* hFile) const;
};

}