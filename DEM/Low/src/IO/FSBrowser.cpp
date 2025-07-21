#include "FSBrowser.h"
#include <IO/IOServer.h>
#include <Data/StringUtils.h>

namespace IO
{

bool CFSBrowser::ForceToFirstEntry()
{
	if (hDir) FS->CloseDirectory(hDir);
	hDir = IOSrv->OpenDirectory(CurrPath.c_str(), nullptr, FS, CurrEntryName, CurrEntryType);
	AtFirstEntry = true;
	return !!hDir;
}
//---------------------------------------------------------------------

bool CFSBrowser::SetRelativePath(const char* pPath)
{
	n_assert(!CurrPath.empty());
	CurrPath = StringUtils::TrimRight(CurrPath, " \r\n\t\\/");
	return SetAbsolutePath((CurrPath + "/" + pPath).c_str());
}
//---------------------------------------------------------------------

bool CFSBrowser::ListCurrDirContents(std::vector<std::string>& OutContents, UPTR EntryTypes, const char* pFilter)
{
	AtFirstEntry = false;

	if (hDir) FS->CloseDirectory(hDir);
	hDir = IOSrv->OpenDirectory(CurrPath.c_str(), pFilter, FS, CurrEntryName, CurrEntryType);
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
