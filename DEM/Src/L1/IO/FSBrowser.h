#pragma once
#ifndef __DEM_L1_FILE_SYSTEM_BROWSER_H__
#define __DEM_L1_FILE_SYSTEM_BROWSER_H__

#include <IO/FileSystem.h>

// Utility class that encapsulates file system browsing session.
// Use it to browse file system(s) and to retrieve directory contents and params.

//!!!can transparently handle mounted archives as folders on directory browsing!

namespace IO
{

class CFSBrowser
{
protected:

	CString			CurrPath;
	PFileSystem		FS;
	void*			hDir;
	bool			AtFirstEntry;
	CString			CurrEntryName;
	EFSEntryType	CurrEntryType;

	bool ForceToFirstEntry();

public:

	CFSBrowser(): hDir(NULL), AtFirstEntry(false) {}
	~CFSBrowser() { Close(); }

	bool			SetAbsolutePath(const CString& Path) { CurrPath = Path; return ForceToFirstEntry(); }
	bool			SetRelativePath(const CString& Path);
	void			Close() { if (hDir) FS->CloseDirectory(hDir); }

	bool			FirstCurrDirEntry() { return hDir && (AtFirstEntry || ForceToFirstEntry()); }
	bool			NextCurrDirEntry();

	bool			ListCurrDirContents(CArray<CString>& OutContents, DWORD EntryTypes = FSE_DIR | FSE_FILE, const CString& Filter = "*");

	bool			IsCurrPathValid() const { return !!hDir; }
	bool			IsCurrDirEmpty();
	const CString&	GetCurrEntryName() const { n_assert(hDir); return CurrEntryName; }
	EFSEntryType	GetCurrEntryType() const { n_assert(hDir); return CurrEntryType; }
	bool			IsCurrEntryFile() const { return hDir && CurrEntryType == FSE_FILE;}
	bool			IsCurrEntryDir() const { return hDir && CurrEntryType == FSE_DIR;}
	PFileSystem		GetFileSystem() const { return FS; }
	const CString&	GetCurrentPath() const { return CurrPath; }
};

}

#endif