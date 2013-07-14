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
		nString					Filter;

		CNPKDir(CNpkTOCEntry* pEntry): pTOCEntry(pEntry), It(pTOCEntry->GetEntryIterator()) {}
	};

	CNpkTOC		TOC;
	CFileStream NPKData; //!!!can use MMF and create views when big files are read!

public:

	virtual ~CFileSystemNPK() { if (NPKData.IsOpen()) Unmount(); }

	virtual bool	Mount(const nString& Source, const nString& Root);
	virtual void	Unmount();
	virtual bool	ProvidesFileCursor() { OK; }

	virtual bool	FileExists(const nString& Path);
	virtual bool	IsFileReadOnly(const nString& Path);
	virtual bool	SetFileReadOnly(const nString& Path, bool ReadOnly) { OK; /*Already read-only*/ }
	virtual bool	DeleteFile(const nString& Path);
	virtual bool	CopyFile(const nString& SrcPath, const nString& DestPath);
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