#pragma once
#include <IO/FileSystem.h>
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
		UPTR			Offset;
	};

	struct CNPKDir
	{
		CNpkTOCEntry*			pTOCEntry;
		CNpkTOCEntry::CIterator	It;
		CString					Filter;

		CNPKDir(CNpkTOCEntry* pEntry): pTOCEntry(pEntry), It(pTOCEntry->GetEntryIterator()) {}

		bool IsValid() const { return pTOCEntry && It != pTOCEntry->End(); }

		operator bool() const { return IsValid(); }
	};

	CNpkTOC	TOC;
	PStream	NPKStream; //!!!can use MMF and create views when big files are read!

public:

	CFileSystemNPK(IStream* pSource);
	virtual ~CFileSystemNPK();

	virtual bool	Init() override;
	virtual bool	IsCaseSensitive() const override { FAIL; }
	virtual bool	IsReadOnly() const { OK; }
	virtual bool	ProvidesFileCursor() const { OK; }

	virtual bool	FileExists(const char* pPath);
	virtual bool	IsFileReadOnly(const char* pPath);
	virtual bool	SetFileReadOnly(const char* pPath, bool ReadOnly) { OK; /*Already read-only*/ }
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
	virtual U64		GetFileWriteTime(void* hFile) const { return 0; }
	virtual bool	Seek(void* hFile, I64 Offset, ESeekOrigin Origin);
	virtual U64		Tell(void* hFile) const;
	virtual bool	Truncate(void* hFile) override;
	virtual void	Flush(void* hFile);
	virtual bool	IsEOF(void* hFile) const;
};

}
