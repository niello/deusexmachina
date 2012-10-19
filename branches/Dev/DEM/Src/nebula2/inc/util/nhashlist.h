#ifndef N_HASHLIST_H
#define N_HASHLIST_H

#include "util/nhashnode.h"

// A doubly linked list of named nodes with fast hashtable based search.
// (C) 2002 RadonLabs GmbH

class nHashList: public nList
{
private:

	enum { N_DEFAULT_HASHSIZE = 16 };

	nHashTable HashTable;

public:

	nHashList(): HashTable(N_DEFAULT_HASHSIZE) {}
	nHashList(int HashSize): HashTable(HashSize) {}

	void		AddHead(nHashNode* pNode);
	void		AddTail(nHashNode* pNode);
	nHashNode*	RemHead();
	nHashNode*	RemTail();
	nHashNode*	GetHead() const { return (nHashNode*)nList::GetHead(); }
	nHashNode*	GetTail() const { return (nHashNode*)nList::GetTail(); }
	nHashNode*	Find(const char* pName) const;
};

inline void nHashList::AddHead(nHashNode* pNode)
{
	n_assert(pNode);
	pNode->SetHashTable(&HashTable);
	HashTable.Add(&pNode->StrNode);
	nList::AddHead((nNode*)pNode);
}
//---------------------------------------------------------------------

inline void nHashList::AddTail(nHashNode* pNode)
{
	n_assert(pNode);
	pNode->SetHashTable(&HashTable);
	HashTable.Add(&pNode->StrNode);
	nList::AddTail((nNode*)pNode);
}
//---------------------------------------------------------------------

inline nHashNode* nHashList::RemHead()
{
	nHashNode* pNode = (nHashNode*)nList::RemHead();
	if (pNode)
	{
		pNode->StrNode.Remove();
		pNode->SetHashTable(NULL);
	}
	return pNode;
}
//---------------------------------------------------------------------

inline nHashNode* nHashList::RemTail()
{
	nHashNode* pNode = (nHashNode*)nList::RemTail();
	if (pNode)
	{
		pNode->StrNode.Remove();
		pNode->SetHashTable(NULL);
	}
	return pNode;
}
//---------------------------------------------------------------------

inline nHashNode* nHashList::Find(const char* pName) const
{
	nStrNode* pNode = HashTable.Find(pName);
	return pNode ? (nHashNode*)pNode->GetPtr() : NULL;
}
//---------------------------------------------------------------------

#endif
