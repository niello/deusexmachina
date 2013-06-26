#ifndef N_STRNODE_H
#define N_STRNODE_H

#include "util/ndatanode.h"
#include "util/nstring.h"

// A node in a nStrList.
// (C) 2002 RadonLabs GmbH

class nStrNode: public nDataNode
{
private:

	nString Name;

public:

	nStrNode() {}
	nStrNode(void* _ptr): nDataNode(_ptr) {}
	nStrNode(const char* str): Name(str) {}
	nStrNode(const char* str, void* _ptr): nDataNode(_ptr), Name(str) {}

	void		SetName(const char* str) { Name.Set(str); }
	const char*	GetName() const { return Name.IsValid() ? Name.CStr() : NULL; } //!!!ONLY while Get returns "" as empty!
	int			GetNameLength() const { return Name.Length(); }

	nStrNode*	GetSucc() const { return (nStrNode*)nNode::GetSucc(); }
	nStrNode*	GetPred() const { return (nStrNode*)nNode::GetPred(); }
};

#endif
