#pragma once
#ifndef __DEM_L1_FILE_SYSTEM_NPK_H__
#define __DEM_L1_FILE_SYSTEM_NPK_H__

#include <IO/Streams/FileStream.h>
#include "NpkTOC.h"

// Original Nebula 2 NPK virtual file system.

namespace IO
{

class CFileSystemNPK: public IFileSystem
{
protected:

	struct CNPKFile
	{
		CNpkTOCEntry*	pTOCEntry;
		DWORD			Offset;
	};

	struct CNPKDir
	{
		CNpkTOCEntry*			pTOCEntry;
		CNpkTOCEntry::CIterator	It;
		CString					Filter;

		CNPKDir(CNpkTOCEntry* pEntry): pTOCEntry(pEntry), It(pTOCEntry->GetEntryIterator()) {}
	};

	CNpkTOC		TOC;
	CFileStream NPKData; //!!!can use MMF and create views when big files are read!

public:

	virtual ~CFileSystemNPK() { if (NPKData.IsOpen()) Unmount(); }

	virtual bool	Mount(const char* pSource, const char* pRoot);
	virtual void	Unmount();
	virtual bool	ProvidesFileCursor() { OK; }

	virtual bool	FileExists(const char* pPath);
	virtual bool	IsFileReadOnly(const char* pPath);
	virtual bool	SetFileReadOnly(const char* pPath, bool ReadOnly) { OK; /*Already read-only*/ }
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
	virtual DWORD	Read(void* hFile, void* pData, DWORD Size);
	virtual DWORD	Write(void* hFile, const void* pData, DWORD Size);
	virtual DWORD	GetFileSize(void* hFile) const;
	virtual DWORD	GetFileWriteTime(void* hFile) const { return 0; }
	virtual bool	Seek(void* hFile, int Offset, ESeekOrigin Origin);
	virtual DWORD	Tell(void* hFile) const;
	virtual void	Flush(void* hFile);
	virtual bool	IsEOF(void* hFile) const;
};

}

#endif