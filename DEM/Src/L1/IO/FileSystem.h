#pragma once
#ifndef __DEM_L1_FILE_SYSTEM_H__
#define __DEM_L1_FILE_SYSTEM_H__

#include <Core/Object.h>
#include <IO/IOFwd.h>

// File system interface. Implementation can be real OS file system or any VFS.
// NB: file times are seconds from 1970-01-01

namespace IO
{

class IFileSystem: public Core::CObject
{
public:

	virtual ~IFileSystem() {}

	virtual bool	Mount(const char* pSource, const char* pRoot) = 0;
	virtual void	Unmount() = 0;
	virtual bool	ProvidesFileCursor() = 0;

	virtual bool	FileExists(const char* pPath) = 0;
	virtual bool	IsFileReadOnly(const char* pPath) = 0;
	virtual bool	SetFileReadOnly(const char* pPath, bool ReadOnly) = 0;
	virtual bool	DeleteFile(const char* pPath) = 0;
	virtual bool	CopyFile(const char* pSrcPath, const char* pDestPath) = 0;
	virtual bool	DirectoryExists(const char* pPath) = 0;
	virtual bool	CreateDirectory(const char* pPath) = 0;
	virtual bool	DeleteDirectory(const char* pPath) = 0;
	virtual bool	GetSystemFolderPath(ESystemFolder Code, CString& OutPath) = 0;

	virtual void*	OpenDirectory(const char* pPath, const char* pFilter, CString& OutName, EFSEntryType& OutType) = 0;
	virtual void	CloseDirectory(void* hDir) = 0;
	virtual bool	NextDirectoryEntry(void* hDir, CString& OutName, EFSEntryType& OutType) = 0;

	virtual void*	OpenFile(const char* pPath, EStreamAccessMode Mode, EStreamAccessPattern Pattern = SAP_DEFAULT) = 0;
	virtual void	CloseFile(void* hFile) = 0;
	virtual UPTR	Read(void* hFile, void* pData, UPTR Size) = 0;
	virtual UPTR	Write(void* hFile, const void* pData, UPTR Size) = 0;
	virtual U64		GetFileSize(void* hFile) const = 0;
	virtual U64		GetFileWriteTime(void* hFile) const = 0;
	virtual bool	Seek(void* hFile, I64 Offset, ESeekOrigin Origin) = 0;
	virtual U64		Tell(void* hFile) const = 0;
	virtual void	Flush(void* hFile) = 0; //???flush MMF views too right here?
	virtual bool	IsEOF(void* hFile) const = 0;

	//???store views in file structures or mb better to have dictionary view_ptr->needed_info?
	//!!!can reuse view if second request of already mapped & viewed region. Adjust returned pointer!
	//virtual void*	MapFile(void* hFile, U64 Offset = 0, UPTR Size = 0) = 0;
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