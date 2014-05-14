#pragma once
#ifndef __DEM_L1_FILE_SYSTEM_WIN32_H__
#define __DEM_L1_FILE_SYSTEM_WIN32_H__

#include <IO/FileSystem.h>
#include <Data/Singleton.h>

// Win32 file system wrapper. It is always created as singleton.

namespace IO
{

class CFileSystemWin32: public IFileSystem
{
	__DeclareSingleton(CFileSystemWin32);

public:

	CFileSystemWin32();
	virtual ~CFileSystemWin32();

	virtual bool	Mount(const CString& Source, const CString& Root) { OK; }
	virtual void	Unmount() {}
	virtual bool	ProvidesFileCursor() { OK; }

	virtual bool	FileExists(const CString& Path);
	virtual bool	IsFileReadOnly(const CString& Path);
	virtual bool	SetFileReadOnly(const CString& Path, bool ReadOnly);
	virtual bool	DeleteFile(const CString& Path);
	virtual bool	CopyFile(const CString& SrcPath, const CString& DestPath);
	virtual bool	DirectoryExists(const CString& Path);
	virtual bool	CreateDirectory(const CString& Path);
	virtual bool	DeleteDirectory(const CString& Path);
	virtual bool	GetSystemFolderPath(ESystemFolder Code, CString& OutPath);

	virtual void*	OpenDirectory(const CString& Path, const CString& Filter, CString& OutName, EFSEntryType& OutType);
	virtual void	CloseDirectory(void* hDir);
	virtual bool	NextDirectoryEntry(void* hDir, CString& OutName, EFSEntryType& OutType);

	virtual void*	OpenFile(const CString& Path, EStreamAccessMode Mode, EStreamAccessPattern Pattern = SAP_DEFAULT);
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