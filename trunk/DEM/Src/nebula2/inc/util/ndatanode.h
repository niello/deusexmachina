#ifndef N_DATA_NODE_H
#define N_DATA_NODE_H
//------------------------------------------------------------------------------
/**
    @class nDataNode
    @ingroup NebulaDataTypes

    @brief nNode with custom user data pointer.

    (C) 2012 Niello
*/
#include "util/nnode.h"

class nDataNode: public nNode
{
private:

	void* ptr;

public:

	nDataNode(): ptr(NULL) {}
	nDataNode(void* _ptr): ptr(_ptr) {}
	
	void	SetPtr(void* p) { ptr = p; }
	void*	GetPtr() const { return ptr; }
};

#endif
