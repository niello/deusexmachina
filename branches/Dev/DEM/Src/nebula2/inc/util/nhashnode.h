#ifndef N_HASHNODE_H
#define N_HASHNODE_H

#include "util/nstrnode.h"
#include "util/nhashtable.h"

// A node element in a nHashList.
// (C) 2002 RadonLabs GmbH

class nHashNode: public nDataNode
{
private:

	friend class nHashList;

	nStrNode	StrNode;
	nHashTable*	pHashTable;

public:

	nHashNode(): StrNode((void*)this), pHashTable(NULL) {}
	nHashNode(const char* name): StrNode(name, (void*)this), pHashTable(NULL) {}

	void		Remove() { StrNode.Remove(); nNode::Remove(); pHashTable = NULL; }

	void		SetHashTable(nHashTable* pTable) { pHashTable = pTable; } // Can be NULL
	const char*	GetName() const { return StrNode.GetName(); }
	void		SetName(const char* pName);
	nHashNode*	GetSucc() const { return (nHashNode*)nNode::GetSucc(); }
	nHashNode*	GetPred() const { return (nHashNode *)nNode::GetPred(); }
};

inline void nHashNode::SetName(const char* pName)
{
	if (IsLinked())
	{
		n_assert(pHashTable);
		StrNode.Remove();
		StrNode.SetName(pName);
		pHashTable->Add(&StrNode);
	}
	else StrNode.SetName(pName);
}
//---------------------------------------------------------------------

#endif
