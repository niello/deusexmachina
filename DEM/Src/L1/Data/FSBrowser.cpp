#include "FSBrowser.h"

namespace Data
{

bool CFSBrowser::ForceToFirstEntry()
{
	if (hDir) FS->CloseDirectory(hDir);
	hDir = DataSrv->OpenDirectory(CurrPath, NULL, FS, CurrEntryName, CurrEntryType);
	AtFirstEntry = true;
	return !!hDir;
}
//---------------------------------------------------------------------

bool CFSBrowser::SetRelativePath(const nString& Path)
{
	n_assert(CurrPath.IsValid());
	CurrPath.StripTrailingSlash();
	return SetAbsolutePath(CurrPath + "/" + Path);
}
//---------------------------------------------------------------------

bool CFSBrowser::ListCurrDirContents(nArray<nString>& OutContents, DWORD EntryTypes, const nString& Filter)
{
	AtFirstEntry = false;

	if (hDir) FS->CloseDirectory(hDir);
	hDir = DataSrv->OpenDirectory(CurrPath, Filter, FS, CurrEntryName, CurrEntryType);
	if (!hDir) FAIL;

	while (CurrEntryType != FSE_NONE)
	{
		if (EntryTypes & CurrEntryType) OutContents.Append(CurrEntryName);
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
