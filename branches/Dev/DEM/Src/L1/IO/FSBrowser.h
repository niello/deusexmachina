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

	nString			CurrPath;
	PFileSystem		FS;
	void*			hDir;
	bool			AtFirstEntry;
	nString			CurrEntryName;
	EFSEntryType	CurrEntryType;

	bool ForceToFirstEntry();

public:

	CFSBrowser(): hDir(NULL), AtFirstEntry(false) {}
	~CFSBrowser() { Close(); }

	bool			SetAbsolutePath(const nString& Path) { CurrPath = Path; return ForceToFirstEntry(); }
	bool			SetRelativePath(const nString& Path);
	void			Close() { if (hDir) FS->CloseDirectory(hDir); }

	bool			FirstCurrDirEntry() { return hDir && (AtFirstEntry || ForceToFirstEntry()); }
	bool			NextCurrDirEntry();

	bool			ListCurrDirContents(nArray<nString>& OutContents, DWORD EntryTypes = FSE_DIR | FSE_FILE, const nString& Filter = "*");

	bool			IsCurrPathValid() const { return !!hDir; }
	bool			IsCurrDirEmpty();
	const nString&	GetCurrEntryName() const { n_assert(hDir); return CurrEntryName; }
	EFSEntryType	GetCurrEntryType() const { n_assert(hDir); return CurrEntryType; }
};

}

#endif