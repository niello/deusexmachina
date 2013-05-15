#pragma once
#ifndef __DEM_L1_FILE_SYSTEM_WIN32_H__
#define __DEM_L1_FILE_SYSTEM_WIN32_H__

#include <IO/FileSystem.h>
#include <Core/Singleton.h>

// Win32 file system wrapper. It is always created as singleton.

namespace IO
{

class CFileSystemWin32: public IFileSystem
{
	__DeclareSingleton(CFileSystemWin32);

public:

	CFileSystemWin32();
	virtual ~CFileSystemWin32();

	virtual bool	Mount(const nString& Source, const nString& Root) { OK; }
	virtual void	Unmount() {}
	virtual bool	ProvidesFileCursor() { OK; }

	virtual bool	FileExists(const nString& Path);
	virtual bool	IsFileReadOnly(const nString& Path);
	virtual bool	SetFileReadOnly(const nString& Path, bool ReadOnly);
	virtual bool	DeleteFile(const nString& Path);
	virtual bool	DirectoryExists(const nString& Path);
	virtual bool	CreateDirectory(const nString& Path);
	virtual bool	DeleteDirectory(const nString& Path);
	virtual bool	GetSystemFolderPath(ESystemFolder Code, nString& OutPath);

	virtual void*	OpenDirectory(const nString& Path, const nString& Filter, nString& OutName, EFSEntryType& OutType);
	virtual void	CloseDirectory(void* hDir);
	virtual bool	NextDirectoryEntry(void* hDir, nString& OutName, EFSEntryType& OutType);

	virtual void*	OpenFile(const nString& Path, EStreamAccessMode Mode, EStreamAccessPattern Pattern = SAP_DEFAULT);
	virtual void	CloseFile(void* hFile);
	virtual DWORD	Read(void* hFile, void* pData, DWORD Size);
	virtual DWORD	Write(void* hFile, const void* pData, DWORD Size);
	virtual DWORD	GetFileSize(void* hFile) const;
	virtual bool	Seek(void* hFile, int Offset, ESeekOrigin Origin);
	virtual DWORD	Tell(void* hFile) const;
	virtual void	Flush(void* hFile);
	virtual bool	IsEOF(void* hFile) const;
};

}

#endif