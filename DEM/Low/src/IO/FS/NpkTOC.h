#pragma once
#ifndef __DEM_L1_NPK_TOC_H__
#define __DEM_L1_NPK_TOC_H__

#include "NpkTOCEntry.h"
#include <Data/String.h>
#include <Data/StringTokenizer.h>
#include <Data/StringUtils.h>

// Holds table of content entries for NPK files.

namespace IO
{

class CNpkTOC
{
private:

	enum { STACKSIZE = 16 };

	CNpkTOCEntry*	pRootDir;      // the top level directory
	CNpkTOCEntry*	pCurrDir;       // current directory (only valid during BeginDirEntry());
	int				CurrStackIdx;
	CNpkTOCEntry*	Stack[STACKSIZE];

	void			Push(CNpkTOCEntry* pEntry) { n_assert(CurrStackIdx < STACKSIZE); Stack[CurrStackIdx++] = pEntry; }
	CNpkTOCEntry*	Pop() { n_assert(CurrStackIdx > 0); return Stack[--CurrStackIdx]; }

public:

	CNpkTOC(): pRootDir(NULL), pCurrDir(NULL), CurrStackIdx(0) { /*memset(Stack, 0, sizeof(Stack));*/ }
	~CNpkTOC() { if (pRootDir) n_delete(pRootDir); }

	CNpkTOCEntry*	BeginDirEntry(const char* pDirName);
	CNpkTOCEntry*	AddFileEntry(const char* pName, UPTR Offset, UPTR Length);
	void			EndDirEntry() { n_assert(pCurrDir); pCurrDir = Pop(); }

	CNpkTOCEntry*	FindEntry(const char* pPath);

	CNpkTOCEntry*	GetRootEntry() const { return pRootDir; }
	CNpkTOCEntry*	GetCurrentDirEntry() { return pCurrDir; }
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

	if (!StringUtils::AreEqualCaseInsensitive(pCurrEntry->GetName(), StrTok.GetNextToken("/\\"))) return nullptr;

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

#endif
