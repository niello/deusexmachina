#ifndef N_LIST_H
#define N_LIST_H
//------------------------------------------------------------------------------
/**
    @class nList
    @ingroup NebulaDataTypes

    @brief Implement a doubly linked list.

    (C) 2002 RadonLabs GmbH
*/
#include "kernel/ntypes.h"
#include "kernel/ndebug.h"
#include "util/nnode.h"

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

	void	AddHead(nNode* n) { n->InsertAfter((nNode*)&(head)); }
	void	AddTail(nNode* n) { n->InsertBefore((nNode*)&(tail)); }
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
	nNode* n = head;
	if (n->succ)
	{
		n->Remove();
		return n;
	}
	return NULL;
}
//---------------------------------------------------------------------

inline nNode* nList::RemTail()
{
	nNode* n = tailpred;
	if (n->pred)
	{
		n->Remove();
		return n;
	}
	return NULL;
}
//---------------------------------------------------------------------

#endif
