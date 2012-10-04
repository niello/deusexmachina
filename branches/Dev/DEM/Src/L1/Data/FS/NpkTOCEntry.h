#pragma once
#ifndef __DEM_L1_NPK_TOC_ENTRY_H__
#define __DEM_L1_NPK_TOC_ENTRY_H__

#include <Data/FileSystem.h>
#include <util/nhashlist.h>

// A table of content entry in a CNpkTOC object. Toc entries are organized
// in a tree of nodes. A node can be a dir node, or a file node. File nodes
// never have children, dir nodes can have children but don't have to.
// (C) 2002 RadonLabs GmbH
// Mofified by Niello (C) 2012

namespace Data
{

class CNpkTOCEntry: public nHashNode
{
private:

	const char*		pRoot;		// Root path string (not owned!)
	CNpkTOCEntry*	pParent;
	EFSEntryType	Type;
	union
	{
		struct
		{
			int		Offset;
			int		Length;
		};
		nHashList*	pEntries;
	};

public:

	CNpkTOCEntry(const char* pRoot, CNpkTOCEntry* parentEntry, const char* pName);
	CNpkTOCEntry(const char* pRoot, CNpkTOCEntry* parentEntry, const char* pName, int FileOffset, int FileLength);
	~CNpkTOCEntry();

	CNpkTOCEntry*	AddDirEntry(const char* pName);
	CNpkTOCEntry*	AddFileEntry(const char* pName, int FileOffset, int FileLength);
	CNpkTOCEntry*	FindEntry(const char* name);
	CNpkTOCEntry*	GetFirstEntry();
	CNpkTOCEntry*	GetNextEntry(CNpkTOCEntry* pCurrEntry);
	nString			GetFullName();

	CNpkTOCEntry*	GetParent() const { return pParent; }
	EFSEntryType	GetType() const { return Type; }
	int				GetFileOffset() const { n_assert(Type == FSE_FILE); return Offset; }
	int				GetFileLength() const { n_assert(Type == FSE_FILE); return Length; }
	const char*		GetRootPath() const { return pRoot; }
};

inline CNpkTOCEntry::CNpkTOCEntry(const char* pRootPath, CNpkTOCEntry* parentEntry, const char* pName) :
	nHashNode(pName),
	pRoot(pRootPath),
	pParent(parentEntry),
	Type(FSE_DIR),
	pEntries(NULL)
{
}
//---------------------------------------------------------------------

inline CNpkTOCEntry::CNpkTOCEntry(const char* pRootPath, CNpkTOCEntry* parentEntry, const char* pName, int FileOffset, int FileLength) :
	nHashNode(pName),
	pRoot(pRootPath),
	pParent(parentEntry),
	Type(FSE_FILE),
	Offset(FileOffset),
	Length(FileLength)
{
}
//---------------------------------------------------------------------

inline CNpkTOCEntry::~CNpkTOCEntry()
{
	if (Type == FSE_DIR && pEntries)
	{
		CNpkTOCEntry* pCurrEntry;
		while (pCurrEntry = (CNpkTOCEntry*)pEntries->RemHead())
			n_delete(pCurrEntry);
		n_delete(pEntries);
	}
}
//---------------------------------------------------------------------

inline CNpkTOCEntry* CNpkTOCEntry::AddDirEntry(const char* pName)
{
	n_assert(Type == FSE_DIR);
	if (!pEntries) pEntries = n_new(nHashList(32));
	CNpkTOCEntry* pNew = n_new(CNpkTOCEntry(GetRootPath(), this, pName));
	pEntries->AddTail(pNew);
	return pNew;
}
//---------------------------------------------------------------------

inline CNpkTOCEntry* CNpkTOCEntry::AddFileEntry(const char* pName, int FileOffset, int FileLength)
{
	n_assert(Type == FSE_DIR);
	if (!pEntries) pEntries = n_new(nHashList(32));
	CNpkTOCEntry* pNew = n_new(CNpkTOCEntry(GetRootPath(), this, pName, FileOffset, FileLength));
	pEntries->AddTail(pNew);
	return pNew;
}
//---------------------------------------------------------------------

inline CNpkTOCEntry* CNpkTOCEntry::FindEntry(const char* pName)
{
	n_assert(Type == FSE_DIR);
	return pEntries ? (CNpkTOCEntry*)pEntries->Find(pName) : NULL;
}
//---------------------------------------------------------------------

inline CNpkTOCEntry* CNpkTOCEntry::GetFirstEntry()
{
	n_assert(Type == FSE_DIR);
	return pEntries ? (CNpkTOCEntry*)pEntries->GetHead() : NULL;
}
//---------------------------------------------------------------------

inline CNpkTOCEntry* CNpkTOCEntry::GetNextEntry(CNpkTOCEntry* pCurrEntry)
{
	n_assert(Type == FSE_DIR && pCurrEntry);
	return (CNpkTOCEntry*)pCurrEntry->GetSucc();
}
//---------------------------------------------------------------------

inline nString CNpkTOCEntry::GetFullName()
{
	const int MaxDepth = 16;
	CNpkTOCEntry* TraceStack[MaxDepth];

	int Depth = 0;
	CNpkTOCEntry* pCurrEntry = this;
	while (pCurrEntry && (Depth < MaxDepth))
	{
		TraceStack[Depth++] = pCurrEntry;
		pCurrEntry = pCurrEntry->GetParent();
	}

	nString Result;
	if (pRoot)
	{
		Result = pRoot;
		Result.Append("/");
	}

	for (--Depth; Depth >= 0; --Depth)
	{
		Result.Append(TraceStack[Depth]->GetName());
		if (Depth > 0) Result.Append("/");
	}
	return Result;
}
//---------------------------------------------------------------------

}

#endif
