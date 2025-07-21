#pragma once
#include <IO/FileSystem.h>

// Utility class that encapsulates file system browsing session.
// Use it to browse file system(s) and to retrieve directory contents and params.

//!!!can transparently handle mounted archives as folders on directory browsing!

namespace IO
{

class CFSBrowser
{
protected:

	std::string		CurrPath;
	PFileSystem		FS;
	void*			hDir = nullptr;
	bool			AtFirstEntry = false;
	std::string		CurrEntryName;
	EFSEntryType	CurrEntryType = FSE_NONE;

	bool ForceToFirstEntry();

public:

	~CFSBrowser() { Close(); }

	bool			SetAbsolutePath(const char* pPath) { CurrPath = pPath; return ForceToFirstEntry(); }
	bool			SetRelativePath(const char* pPath);
	void			Close() { if (hDir) FS->CloseDirectory(hDir); }

	bool			FirstCurrDirEntry() { return hDir && (AtFirstEntry || ForceToFirstEntry()); }
	bool			NextCurrDirEntry();

	bool			ListCurrDirContents(std::vector<std::string>& OutContents, UPTR EntryTypes = FSE_DIR | FSE_FILE, const char* pFilter = "*");

	bool			IsCurrPathValid() const { return !!hDir; }
	bool			IsCurrDirEmpty();
	const auto&     GetCurrEntryName() const { n_assert(hDir); return CurrEntryName; }
	EFSEntryType	GetCurrEntryType() const { n_assert(hDir); return CurrEntryType; }
	bool			IsCurrEntryFile() const { return hDir && CurrEntryType == FSE_FILE;}
	bool			IsCurrEntryDir() const { return hDir && CurrEntryType == FSE_DIR;}
	PFileSystem		GetFileSystem() const { return FS; }
	const auto&     GetCurrentPath() const { return CurrPath; }
};

}
