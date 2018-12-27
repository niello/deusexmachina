#pragma once
#ifndef __DEM_L1_FILE_SYSTEM_WIN32_H__
#define __DEM_L1_FILE_SYSTEM_WIN32_H__

#include <IO/FileSystem.h>

// Win32 file system wrapper.

namespace IO
{

class CFileSystemWin32: public IFileSystem
{
public:

	virtual bool	Mount(const char* pSource, const char* pRoot) { OK; }
	virtual void	Unmount() {}
	virtual bool	IsReadOnly() const { FAIL; }
	virtual bool	ProvidesFileCursor() const { OK; }

	virtual bool	FileExists(const char* pPath);
	virtual bool	IsFileReadOnly(const char* pPath);
	virtual bool	SetFileReadOnly(const char* pPath, bool ReadOnly);
	virtual bool	DeleteFile(const char* pPath);
	virtual bool	CopyFile(const char* pSrcPath, const char* pDestPath);
	virtual bool	DirectoryExists(const char* pPath);
	virtual bool	CreateDirectory(const char* pPath);
	virtual bool	DeleteDirectory(const char* pPath);
	virtual bool	GetSystemFolderPath(ESystemFolder Code, CString& OutPath);

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

#endif