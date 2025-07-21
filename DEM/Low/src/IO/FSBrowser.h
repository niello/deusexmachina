#pragma once
#ifndef __DEM_L1_FILE_SYSTEM_BROWSER_H__
#define __DEM_L1_FILE_SYSTEM_BROWSER_H__

#include <IO/FileSystem.h>
#include <Data/String.h>

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

	CFSBrowser(): hDir(nullptr), AtFirstEntry(false) {}
	~CFSBrowser() { Close(); }

	bool			SetAbsolutePath(const char* pPath) { CurrPath = pPath; return ForceToFirstEntry(); }
	bool			SetRelativePath(const char* pPath);
	void			Close() { if (hDir) FS->CloseDirectory(hDir); }

	bool			FirstCurrDirEntry() { return hDir && (AtFirstEntry || ForceToFirstEntry()); }
	bool			NextCurrDirEntry();

	bool			ListCurrDirContents(std::vector<CString>& OutContents, UPTR EntryTypes = FSE_DIR | FSE_FILE, const char* pFilter = "*");

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
