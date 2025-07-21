#include "FSBrowser.h"

#include <IO/IOServer.h>

namespace IO
{

bool CFSBrowser::ForceToFirstEntry()
{
	if (hDir) FS->CloseDirectory(hDir);
	hDir = IOSrv->OpenDirectory(CurrPath.CStr(), nullptr, FS, CurrEntryName, CurrEntryType);
	AtFirstEntry = true;
	return !!hDir;
}
//---------------------------------------------------------------------

bool CFSBrowser::SetRelativePath(const char* pPath)
{
	n_assert(CurrPath.IsValid());
	CurrPath.Trim(" \r\n\t\\/", false);
	return SetAbsolutePath((CurrPath + "/" + pPath).CStr());
}
//---------------------------------------------------------------------

bool CFSBrowser::ListCurrDirContents(std::vector<CString>& OutContents, UPTR EntryTypes, const char* pFilter)
{
	AtFirstEntry = false;

	if (hDir) FS->CloseDirectory(hDir);
	hDir = IOSrv->OpenDirectory(CurrPath.CStr(), pFilter, FS, CurrEntryName, CurrEntryType);
	if (!hDir) FAIL;

	while (CurrEntryType != FSE_NONE)
	{
		if (EntryTypes & CurrEntryType) OutContents.push_back(CurrEntryName);
		if (!FS->NextDirectoryEntry(hDir, CurrEntryName, CurrEntryType)) break;
	}

	OK;
}
//---------------------------------------------------------------------

bool CFSBrowser::NextCurrDirEntry()
{
	AtFirstEntry = false;
	return hDir && FS->NextDirectoryEntry(hDir, CurrEntryName, CurrEntryType);
}
//---------------------------------------------------------------------

bool CFSBrowser::IsCurrDirEmpty()
{
	n_assert(hDir);
	if (!AtFirstEntry) n_assert(ForceToFirstEntry());
	return CurrEntryType == FSE_NONE;
}
//---------------------------------------------------------------------

}
