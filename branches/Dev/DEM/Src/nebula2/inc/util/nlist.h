#ifndef N_LIST_H
#define N_LIST_H

#include "kernel/ntypes.h"
#include "util/nnode.h"

// Implement a doubly linked list.
// (C) 2002 RadonLabs GmbH

class nList
{
private:

	nNode* head;
	nNode* tail;
	nNode* tailpred;

public:

	nList();
	nList(const nList& Other);
	~nList() { n_assert(IsEmpty()); }

	bool	IsEmpty() const { return !head->succ; }

	void	AddHead(nNode* pNode) { pNode->InsertAfter((nNode*)&(head)); }
	void	AddTail(nNode* pNode) { pNode->InsertBefore((nNode*)&(tail)); }
	nNode*	RemHead();
	nNode*	RemTail();

	nNode*	GetHead() const { return head->succ ? head : NULL; }
	nNode*	GetTail() const { return tailpred->pred ? tailpred : NULL; }

	void operator =(const nList& Other) { n_assert2(Other.IsEmpty(), "Copying non-empty nLists isn't supported for now!"); }
};
//---------------------------------------------------------------------

inline nList::nList(): tail(NULL)
{
	head = (nNode*)&(tail);
	tailpred = (nNode*)&(head);
}
//---------------------------------------------------------------------

inline nList::nList(const nList& Other): tail(NULL)
{
	n_assert2(Other.IsEmpty(), "Copying non-empty nLists isn't supported for now!");
	head = (nNode*)&(tail);
	tailpred = (nNode*)&(head);
}
//---------------------------------------------------------------------

inline nNode* nList::RemHead()
{
	nNode* pNode = head;
	if (pNode->succ)
	{
		pNode->Remove();
		return pNode;
	}
	return NULL;
}
//---------------------------------------------------------------------

inline nNode* nList::RemTail()
{
	nNode* pNode = tailpred;
	if (pNode->pred)
	{
		pNode->Remove();
		return pNode;
	}
	return NULL;
}
//---------------------------------------------------------------------

#endif
