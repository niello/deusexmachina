#ifndef N_DATA_NODE_H
#define N_DATA_NODE_H

#include "util/nnode.h"

// nNode with custom user data pointer. Whereas nNode is intended to be derived to store user data,
// this node contains arbitrary user data as a pointer.

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
