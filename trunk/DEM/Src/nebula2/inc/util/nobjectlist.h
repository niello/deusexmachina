#ifndef N_OBJECTLIST_H
#define N_OBJECTLIST_H
//------------------------------------------------------------------------------
/**
    @class nObjectList
    @ingroup Util
    @brief A doubly linked list for object nodes.

    (C) 2005 RadonLabs GmbH
*/
#include "util/nlist.h"
#include "util/nobjectnode.h"

template <class TYPE>
class nObjectList: public nList
{
public:

	nObjectNode<TYPE>* GetHead() const { return (nObjectNode<TYPE>*)this->nList::GetHead(); }
	nObjectNode<TYPE>* GetTail() const { return (nObjectNode<TYPE>*)this->nList::GetTail(); }
	nObjectNode<TYPE>* RemHead() { return (nObjectNode<TYPE>*)this->nList::RemHead(); }
	nObjectNode<TYPE>* RemTail() { return (nObjectNode<TYPE>*)this->nList::RemTail(); }
};

#endif
