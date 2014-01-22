#pragma once
#ifndef __DEM_L1_NPK_TOC_H__
#define __DEM_L1_NPK_TOC_H__

#include "NpkTOCEntry.h"
#include <Data/String.h>
#include <Data/StringTokenizer.h>

// Holds table of content entries for NPK files.

namespace IO
{

class CNpkTOC
{
private:

	enum { STACKSIZE = 16 };

	CNpkTOCEntry*	pRootDir;      // the top level directory
	CString			RootPath;           // filesystem pPath to the root pEntry (i.e. d:/nomads)
	CNpkTOCEntry*	pCurrDir;       // current directory (only valid during BeginDirEntry());
	int				CurrStackIdx;
	CNpkTOCEntry*	Stack[STACKSIZE];

	void			Push(CNpkTOCEntry* pEntry) { n_assert(CurrStackIdx < STACKSIZE); Stack[CurrStackIdx++] = pEntry; }
	CNpkTOCEntry*	Pop() { n_assert(CurrStackIdx > 0); return Stack[--CurrStackIdx]; }

public:

	CNpkTOC(): pRootDir(NULL), pCurrDir(NULL), CurrStackIdx(0) { /*memset(Stack, 0, sizeof(Stack));*/ }
	~CNpkTOC() { if (pRootDir) n_delete(pRootDir); }

	CNpkTOCEntry*	BeginDirEntry(const char* pDirName);
	CNpkTOCEntry*	AddFileEntry(const char* pName, int Offset, int Length);
	void			EndDirEntry() { n_assert(pCurrDir); pCurrDir = Pop(); }

	CNpkTOCEntry*	FindEntry(const char* name);

	CNpkTOCEntry*	GetRootEntry() const { return pRootDir; }
	CNpkTOCEntry*	GetCurrentDirEntry() { return pCurrDir; }
	void			SetRootPath(const char* pPath) { n_assert(pPath); RootPath = pPath; RootPath.ConvertBackslashes(); RootPath.StripTrailingSlash(); }
	const char*		GetRootPath() const { return RootPath.IsEmpty() ? NULL : RootPath.CStr(); }
};

inline CNpkTOCEntry* CNpkTOC::BeginDirEntry(const char* pDirName)
{
	n_assert_dbg(pDirName);

	CNpkTOCEntry* pEntry;
	if (pCurrDir) pEntry = pCurrDir->AddDirEntry(pDirName);
	else
	{
		n_assert(!pRootDir);
		pEntry = n_new(CNpkTOCEntry(GetRootPath(), 0, pDirName));
		pRootDir = pEntry;
	}

	Push(pCurrDir);
	pCurrDir = pEntry;
	return pEntry;
}
//---------------------------------------------------------------------

inline CNpkTOCEntry* CNpkTOC::AddFileEntry(const char* pName, int Offset, int Length)
{
	n_assert_dbg(pName);
	n_assert(pCurrDir);
	return pCurrDir->AddFileEntry(pName, Offset, Length);
}
//---------------------------------------------------------------------

inline CNpkTOCEntry* CNpkTOC::FindEntry(const char* pAbsPath)
{
	n_assert(pRootDir && RootPath.IsValid());

	if (_strnicmp(RootPath.CStr(), pAbsPath, RootPath.Length())) return NULL;

	DWORD PathLen = strlen(pAbsPath);
	DWORD RootPathLen = RootPath.Length() + 1; // 1 is for a directory separator
	if (PathLen <= RootPathLen) return NULL;
	PathLen = PathLen - RootPathLen + 1; // 1 is for a terminating null
	char* pLocalPath = (char*)_malloca(PathLen);
	memcpy_s(pLocalPath, PathLen, pAbsPath + RootPathLen, PathLen);
	_strlwr_s(pLocalPath, PathLen);

	CNpkTOCEntry* pCurrEntry = pRootDir;

	char Buf[N_MAXPATH];
	Data::CStringTokenizer StrTok(pLocalPath, "/\\", Buf, N_MAXPATH);
	if (pCurrEntry->GetName() != StrTok.GetNextToken())
	{
		_freea(pLocalPath);
		return NULL;
	}

	while (pCurrEntry && !StrTok.IsLast())
	{
		StrTok.GetNextToken();
		if (!strcmp(StrTok.GetCurrToken(), ".."))
			pCurrEntry = pCurrEntry->GetParent();
		else if (strcmp(StrTok.GetCurrToken(), ".")) // NB: if is NOT a dot (dot is a current dir, no action needed)
			pCurrEntry = pCurrEntry->FindEntry(StrTok.GetCurrToken());
	}

	_freea(pLocalPath);
	return pCurrEntry;
}
//---------------------------------------------------------------------

}

#endif
