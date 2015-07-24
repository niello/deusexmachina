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

	virtual bool	Mount(const CString& Source, const CString& Root);
	virtual void	Unmount();
	virtual bool	ProvidesFileCursor() { OK; }

	virtual bool	FileExists(const CString& Path);
	virtual bool	IsFileReadOnly(const CString& Path);
	virtual bool	SetFileReadOnly(const CString& Path, bool ReadOnly) { OK; /*Already read-only*/ }
	virtual bool	DeleteFile(const CString& Path);
	virtual bool	CopyFile(const CString& SrcPath, const CString& DestPath);
	virtual bool	DirectoryExists(const CString& Path);
	virtual bool	CreateDirectory(const CString& Path);
	virtual bool	DeleteDirectory(const CString& Path);
	virtual bool	GetSystemFolderPath(ESystemFolder Code, CString& OutPath);

	virtual void*	OpenDirectory(const CString& Path, const char* pFilter, CString& OutName, EFSEntryType& OutType);
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