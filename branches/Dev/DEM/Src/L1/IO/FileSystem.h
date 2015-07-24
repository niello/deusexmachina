#pragma once
#ifndef __DEM_L1_FILE_SYSTEM_H__
#define __DEM_L1_FILE_SYSTEM_H__

#include <Core/Object.h>
#include <IO/IOFwd.h>

// File system interface. Implementation can be real OS file system or any VFS.

namespace IO
{

class IFileSystem: public Core::CObject
{
public:

	virtual ~IFileSystem() {}

	virtual bool	Mount(const CString& Source, const CString& Root) = 0;
	virtual void	Unmount() = 0;
	virtual bool	ProvidesFileCursor() = 0;

	virtual bool	FileExists(const CString& Path) = 0;
	virtual bool	IsFileReadOnly(const CString& Path) = 0;
	virtual bool	SetFileReadOnly(const CString& Path, bool ReadOnly) = 0;
	virtual bool	DeleteFile(const CString& Path) = 0;
	virtual bool	CopyFile(const CString& SrcPath, const CString& DestPath) = 0;
	virtual bool	DirectoryExists(const CString& Path) = 0;
	virtual bool	CreateDirectory(const CString& Path) = 0;
	virtual bool	DeleteDirectory(const CString& Path) = 0;
	virtual bool	GetSystemFolderPath(ESystemFolder Code, CString& OutPath) = 0;

	virtual void*	OpenDirectory(const CString& Path, const char* pFilter, CString& OutName, EFSEntryType& OutType) = 0;
	virtual void	CloseDirectory(void* hDir) = 0;
	virtual bool	NextDirectoryEntry(void* hDir, CString& OutName, EFSEntryType& OutType) = 0;

	virtual void*	OpenFile(const CString& Path, EStreamAccessMode Mode, EStreamAccessPattern Pattern = SAP_DEFAULT) = 0;
	virtual void	CloseFile(void* hFile) = 0;
	virtual DWORD	Read(void* hFile, void* pData, DWORD Size) = 0;
	virtual DWORD	Write(void* hFile, const void* pData, DWORD Size) = 0;
	virtual DWORD	GetFileSize(void* hFile) const = 0;
	virtual bool	Seek(void* hFile, int Offset, ESeekOrigin Origin) = 0;
	virtual DWORD	Tell(void* hFile) const = 0;
	virtual void	Flush(void* hFile) = 0; //???flush MMF views too right here?
	virtual bool	IsEOF(void* hFile) const = 0;

	//???store views in file structures or mb better to have dictionary view_ptr->needed_info?
	//!!!can reuse view if second request of already mapped & viewed region. Adjust returned pointer!
	//virtual void*	MapFile(void* hFile, DWORD Offset = 0, DWORD Size = 0) = 0; //!!!need int64 for offset & size everywhere!
	//virtual void	UnmapFile(void* hFile) = 0; //!!!if multiview, need to specify view handle!
};

typedef Ptr<IFileSystem> PFileSystem;

/*
    void SetFileWriteTime(const Util::String& path, IO::FileTime fileTime);
    IO::FileTime GetFileWriteTime(const Util::String& path);
    Util::Array<Util::String> ListFiles(const Util::String& dirPath, const Util::String& pattern);
    Util::Array<Util::String> ListDirectories(const Util::String& dirPath, const Util::String& pattern);
    /// return true when the string is a device name (e.g. "C:")
    bool IsDeviceName(const Util::String& str);
*/
}

#endif