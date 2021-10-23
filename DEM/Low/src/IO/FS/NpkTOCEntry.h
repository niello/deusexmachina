#pragma once
#include <IO/FileSystem.h>
#include <Data/StringID.h>
#include <map>

// A table of content entry in a CNpkTOC object. Toc entries are organized
// in a tree of nodes. A node can be a dir node, or a file node. File nodes
// never have children, dir nodes can have children but don't have to.

namespace IO
{

class CNpkTOCEntry
{
public:

	typedef std::map<CStrID, CNpkTOCEntry*, std::less<>> CEntryTable;
	typedef CEntryTable::iterator CIterator;

private:

	CStrID				Name;
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
	CNpkTOCEntry*				FindEntry(const char* pName) const;
	CIterator					GetEntryIterator() { return pEntries ? pEntries->begin() : CIterator{}; }
	CIterator					End() const { return pEntries ? pEntries->end() : CIterator{}; }

	CStrID                      GetName() const { return Name; }
	std::string					GetFullName() const;
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
		for (auto It : *pEntries)
			n_delete(It.second);
		n_delete(pEntries);
	}
}
//---------------------------------------------------------------------

inline CNpkTOCEntry* CNpkTOCEntry::AddDirEntry(const char* pName)
{
	n_assert_dbg(pName);
	n_assert(Type == FSE_DIR);
	if (!pEntries) pEntries = n_new(CEntryTable());
	CNpkTOCEntry* pNew = n_new(CNpkTOCEntry(this, pName));
	pEntries->emplace(pNew->GetName(), pNew);
	return pNew;
}
//---------------------------------------------------------------------

inline CNpkTOCEntry* CNpkTOCEntry::AddFileEntry(const char* pName, UPTR FileOffset, UPTR FileLength)
{
	n_assert_dbg(pName);
	n_assert(Type == FSE_DIR);
	if (!pEntries) pEntries = n_new(CEntryTable());
	CNpkTOCEntry* pNew = n_new(CNpkTOCEntry(this, pName, FileOffset, FileLength));
	pEntries->emplace(pNew->GetName(), pNew);
	return pNew;
}
//---------------------------------------------------------------------

inline CNpkTOCEntry* CNpkTOCEntry::FindEntry(const char* pName) const
{
	n_assert(Type == FSE_DIR);
	if (!pEntries) return nullptr;

	std::string Name(pName);
	std::transform(Name.begin(), Name.end(), Name.begin(), std::tolower);

	auto It = pEntries->find(Name.c_str());
	return (It == pEntries->cend()) ? It->second : nullptr;
}
//---------------------------------------------------------------------

inline std::string CNpkTOCEntry::GetFullName() const
{
	constexpr UPTR MAX_DEPTH = 16;
	const CNpkTOCEntry* TraceStack[MAX_DEPTH];

	int Depth = 0;
	const CNpkTOCEntry* pCurrEntry = this;
	while (pCurrEntry && (Depth < MAX_DEPTH))
	{
		TraceStack[Depth++] = pCurrEntry;
		pCurrEntry = pCurrEntry->GetParent();
	}

	std::string Result;
	for (--Depth; Depth >= 0; --Depth)
	{
		Result += TraceStack[Depth]->GetName().CStr();
		if (Depth > 0) Result += '/';
	}
	return Result;
}
//---------------------------------------------------------------------

}
