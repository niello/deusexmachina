#ifndef N_STRLIST_H
#define N_STRLIST_H

#include <string.h>
#include "util/nlist.h"
#include "util/nstrnode.h"

// A doubly linked list for named nodes with slow linear search.
// (C) 2002 RadonLabs GmbH

class nStrList: public nList
{
public:

	nStrNode* GetHead() const { return (nStrNode*)nList::GetHead(); }
	nStrNode* GetTail() const { return (nStrNode*)nList::GetTail(); }
	nStrNode* RemHead() { return (nStrNode*)nList::RemHead(); }
	nStrNode* RemTail() { return (nStrNode*)nList::RemTail(); }
	nStrNode* Find(const char* pStr) const;
};

inline nStrNode* nStrList::Find(const char* pStr) const
{
	for (nStrNode* pNode = GetHead(); pNode; pNode = pNode->GetSucc())
	{
		const char* pName = pNode->GetName();
		n_assert(pName);
		if (!strcmp(pStr, pName)) return pNode;
	}
	return NULL;
}
//---------------------------------------------------------------------

#endif
