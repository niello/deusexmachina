#pragma once
#ifndef __DEM_L1_NPK_TOC_ENTRY_H__
#define __DEM_L1_NPK_TOC_ENTRY_H__

#include <IO/FileSystem.h>
#include <Data/HashTable.h>
#include <Data/String.h>

// A table of content entry in a CNpkTOC object. Toc entries are organized
// in a tree of nodes. A node can be a dir node, or a file node. File nodes
// never have children, dir nodes can have children but don't have to.

namespace IO
{

class CNpkTOCEntry
{
public:

	typedef CHashTable<CString, CNpkTOCEntry*> CEntryTable;
	typedef CEntryTable::CIterator CIterator;

private:

	CString				Name;
	CNpkTOCEntry*		pParent;
	EFSEntryType		Type;
	union
	{
		struct
		{
			UPTR		Offset;
			UPTR		Length;
		};
		CEntryTable*	pEntries;
	};

public:

	CNpkTOCEntry(CNpkTOCEntry* pParentEntry, const char* pName);
	CNpkTOCEntry(CNpkTOCEntry* pParentEntry, const char* pName, UPTR FileOffset, UPTR FileLength);
	~CNpkTOCEntry();

	CNpkTOCEntry*				AddDirEntry(const char* pName);
	CNpkTOCEntry*				AddFileEntry(const char* pName, UPTR FileOffset, UPTR FileLength);
	CNpkTOCEntry*				FindEntry(const char* pName);
	CIterator					GetEntryIterator() { return pEntries ? pEntries->Begin() : CIterator(nullptr); }

	const CString&	GetName() const { return Name; }
	CString						GetFullName() const;
	CNpkTOCEntry*				GetParent() const { return pParent; }
	EFSEntryType				GetType() const { return Type; }
	bool						IsFile() const { return Type == IO::FSE_FILE; }
	bool						IsDir() const { return Type == IO::FSE_DIR; }
	UPTR						GetFileOffset() const { n_assert(IsFile()); return Offset; }
	UPTR						GetFileLength() const { n_assert(IsFile()); return Length; }
};

inline CNpkTOCEntry::CNpkTOCEntry(CNpkTOCEntry* pParentEntry, const char* pName):
	Name(pName),
	pParent(pParentEntry),
	Type(FSE_DIR),
	pEntries(nullptr)
{
}
//---------------------------------------------------------------------

inline CNpkTOCEntry::CNpkTOCEntry(CNpkTOCEntry* pParentEntry, const char* pName, UPTR FileOffset, UPTR FileLength):
	Name(pName),
	pParent(pParentEntry),
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
		for (CIterator It = pEntries->Begin(); It; ++It)
			n_delete(It.GetValue());
		n_delete(pEntries);
	}
}
//---------------------------------------------------------------------

inline CNpkTOCEntry* CNpkTOCEntry::AddDirEntry(const char* pName)
{
	n_assert_dbg(pName);
	n_assert(Type == FSE_DIR);
	if (!pEntries) pEntries = n_new(CEntryTable(32));
	CNpkTOCEntry* pNew = n_new(CNpkTOCEntry(this, pName));
	pEntries->Add(pNew->GetName(), pNew);
	return pNew;
}
//---------------------------------------------------------------------

inline CNpkTOCEntry* CNpkTOCEntry::AddFileEntry(const char* pName, UPTR FileOffset, UPTR FileLength)
{
	n_assert_dbg(pName);
	n_assert(Type == FSE_DIR);
	if (!pEntries) pEntries = n_new(CEntryTable(32));
	CNpkTOCEntry* pNew = n_new(CNpkTOCEntry(this, pName, FileOffset, FileLength));
	pEntries->Add(pNew->GetName(), pNew);
	return pNew;
}
//---------------------------------------------------------------------

inline CNpkTOCEntry* CNpkTOCEntry::FindEntry(const char* pName)
{
	n_assert(Type == FSE_DIR);
	if (!pEntries) return nullptr;

	CString Name(pName);
	Name.ToLower();

	CNpkTOCEntry* pResult;
	return pEntries->Get(Name, pResult) ? pResult : nullptr;
}
//---------------------------------------------------------------------

inline CString CNpkTOCEntry::GetFullName() const
{
	const int MaxDepth = 16;
	const CNpkTOCEntry* TraceStack[MaxDepth];

	int Depth = 0;
	const CNpkTOCEntry* pCurrEntry = this;
	while (pCurrEntry && (Depth < MaxDepth))
	{
		TraceStack[Depth++] = pCurrEntry;
		pCurrEntry = pCurrEntry->GetParent();
	}

	CString Result;
	for (--Depth; Depth >= 0; --Depth)
	{
		Result.Add(TraceStack[Depth]->GetName());
		if (Depth > 0) Result.Add("/");
	}
	return Result;
}
//---------------------------------------------------------------------

}

#endif
