#ifndef N_NODE_H
#define N_NODE_H

#include "kernel/ntypes.h"

// Implement a node in a doubly linked list.
// (C) 2002 RadonLabs GmbH

class nNode
{
private:

	nNode* succ;
	nNode* pred;

	friend class nList;

public:

	nNode(): succ(NULL), pred(NULL) {}
	~nNode() { n_assert(!succ); }

	nNode*	GetSucc() const { n_assert(succ); return succ->succ ? succ : NULL; }
	nNode*	GetPred() const { n_assert(pred); return pred->pred ? pred : NULL; }

	void	InsertBefore(nNode* succ);
	void	InsertAfter(nNode* pred);
	void	Remove();

	bool	IsLinked() const { return succ != NULL; }
};
//---------------------------------------------------------------------

inline void nNode::InsertBefore(nNode* _succ)
{
	n_assert(_succ->pred);
	n_assert(!succ);
	pred = _succ->pred;
	succ = _succ;
	pred->succ = this;
	_succ->pred = this;
}
//---------------------------------------------------------------------

inline void nNode::InsertAfter(nNode* _pred)
{
	n_assert(_pred->succ);
	n_assert(!succ);
	pred = _pred;
	succ = _pred->succ;
	_pred->succ = this;
	succ->pred = this;
}
//---------------------------------------------------------------------

inline void nNode::Remove()
{
	n_assert(succ);
	succ->pred = pred;
	pred->succ = succ;
	succ = NULL;
	pred = NULL;
}
//---------------------------------------------------------------------

#endif
