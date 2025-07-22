#pragma once
#include "NpkTOCEntry.h"
#include <Data/StringTokenizer.h>
#include <Data/StringUtils.h>

// Holds table of content entries for NPK files.

namespace IO
{

class CNpkTOC
{
private:

	CNpkTOCEntry*	pRootDir = nullptr;	// the top level directory

	// TODO: all 'private' below is needed only at construction time. Optimize it away to stack variables?

	enum { STACKSIZE = 16 };

	CNpkTOCEntry*	pCurrDir = nullptr;	// current directory (only valid during BeginDirEntry());
	int				CurrStackIdx = 0;
	CNpkTOCEntry*	Stack[STACKSIZE];

	void			Push(CNpkTOCEntry* pEntry) { n_assert(CurrStackIdx < STACKSIZE); Stack[CurrStackIdx++] = pEntry; }
	CNpkTOCEntry*	Pop() { n_assert(CurrStackIdx > 0); return Stack[--CurrStackIdx]; }

public:

	~CNpkTOC() { if (pRootDir) n_delete(pRootDir); }

	CNpkTOCEntry*	BeginDirEntry(const char* pDirName);
	CNpkTOCEntry*	AddFileEntry(const char* pName, UPTR Offset, UPTR Length);
	void			EndDirEntry() { n_assert(pCurrDir); pCurrDir = Pop(); }

	CNpkTOCEntry*	FindEntry(const char* pPath);
	CNpkTOCEntry*	GetRootEntry() const { return pRootDir; }
};

inline CNpkTOCEntry* CNpkTOC::BeginDirEntry(const char* pDirName)
{
	n_assert_dbg(pDirName);

	CNpkTOCEntry* pEntry;
	if (pCurrDir) pEntry = pCurrDir->AddDirEntry(pDirName);
	else
	{
		n_assert(!pRootDir);
		pEntry = n_new(CNpkTOCEntry(nullptr, pDirName));
		pRootDir = pEntry;
	}

	Push(pCurrDir);
	pCurrDir = pEntry;
	return pEntry;
}
//---------------------------------------------------------------------

inline CNpkTOCEntry* CNpkTOC::AddFileEntry(const char* pName, UPTR Offset, UPTR Length)
{
	n_assert_dbg(pName);
	n_assert(pCurrDir);
	return pCurrDir->AddFileEntry(pName, Offset, Length);
}
//---------------------------------------------------------------------

inline CNpkTOCEntry* CNpkTOC::FindEntry(const char* pPath)
{
	n_assert(pRootDir);

	if (!pPath) return nullptr;

	CNpkTOCEntry* pCurrEntry = pRootDir;

	char Buf[DEM_MAX_PATH];
	Data::CStringTokenizer StrTok(pPath, Buf, DEM_MAX_PATH);

	if (!StringUtils::AreEqualCaseInsensitive(pCurrEntry->GetName().CStr(), StrTok.GetNextToken("/\\"))) return nullptr;

	while (pCurrEntry && !StrTok.IsLast())
	{
		StrTok.GetNextToken("/\\");
		if (!strcmp(StrTok.GetCurrToken(), ".."))
			pCurrEntry = pCurrEntry->GetParent();
		else if (strcmp(StrTok.GetCurrToken(), ".")) // NB: if is NOT a dot (dot is a current dir, no action needed)
			pCurrEntry = pCurrEntry->FindEntry(StrTok.GetCurrToken());
	}

	return pCurrEntry;
}
//---------------------------------------------------------------------

}
