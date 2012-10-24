#ifndef N_OBJECTLIST_H
#define N_OBJECTLIST_H

#include "util/nlist.h"
#include "util/nobjectnode.h"

// A doubly linked list for object nodes.
// (C) 2005 RadonLabs GmbH

template <class T>
class nObjectList: public nList
{
public:

	nObjectNode<T>* GetHead() const { return (nObjectNode<T>*)nList::GetHead(); }
	nObjectNode<T>* GetTail() const { return (nObjectNode<T>*)nList::GetTail(); }
	nObjectNode<T>* RemHead() { return (nObjectNode<T>*)nList::RemHead(); }
	nObjectNode<T>* RemTail() { return (nObjectNode<T>*)nList::RemTail(); }
};

#endif
