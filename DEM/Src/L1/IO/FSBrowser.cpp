#include "FSBrowser.h"

#include <IO/IOServer.h>

namespace IO
{

bool CFSBrowser::ForceToFirstEntry()
{
	if (hDir) FS->CloseDirectory(hDir);
	hDir = IOSrv->OpenDirectory(CurrPath, NULL, FS, CurrEntryName, CurrEntryType);
	AtFirstEntry = true;
	return !!hDir;
}
//---------------------------------------------------------------------

bool CFSBrowser::SetRelativePath(const CString& Path)
{
	n_assert(CurrPath.IsValid());
	CurrPath.StripTrailingSlash();
	return SetAbsolutePath(CurrPath + "/" + Path);
}
//---------------------------------------------------------------------

bool CFSBrowser::ListCurrDirContents(CArray<CString>& OutContents, DWORD EntryTypes, const CString& Filter)
{
	AtFirstEntry = false;

	if (hDir) FS->CloseDirectory(hDir);
	hDir = IOSrv->OpenDirectory(CurrPath, Filter, FS, CurrEntryName, CurrEntryType);
	if (!hDir) FAIL;

	while (CurrEntryType != FSE_NONE)
	{
		if (EntryTypes & CurrEntryType) OutContents.Add(CurrEntryName);
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
