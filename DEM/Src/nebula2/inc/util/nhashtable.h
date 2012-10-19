#ifndef N_HASHTABLE_H
#define N_HASHTABLE_H

#include "kernel/ntypes.h"
#include "util/nstrlist.h"
#include "util/Hash.h"

// Implements a simple string hash table.
// (C) 2002 RadonLabs GmbH

class nHashTable
{
private:

	nStrList*	Table;
	int			TableSize;

public:

	nHashTable(int Size): TableSize(Size) { Table = n_new_array(nStrList, Size); }
	~nHashTable() { n_delete_array(Table); }

	void		Add(nStrNode* pNode);
	nStrNode*	Find(const char* pStr) const { return Table[Hash(pStr, strlen(pStr)) % TableSize].Find(pStr); }
};

inline void nHashTable::Add(nStrNode* pNode)
{
	n_assert_dbg(pNode);
	Table[Hash(pNode->GetName(), pNode->GetNameLength()) % TableSize].AddHead(pNode);
}
//---------------------------------------------------------------------

#endif
