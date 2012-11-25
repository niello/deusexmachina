#ifndef N_OBJECTNODE_H
#define N_OBJECTNODE_H
//------------------------------------------------------------------------------
/**
    @class nObjectNode
    @ingroup Util
    @brief A node in a nObjectList.

    (C) 2005 RadonLabs GmbH
*/
#include "util/nnode.h"

#ifdef GetObject
#undef GetObject
#endif

template <class T>
class nObjectNode: public nNode
{
public:

	T Object;

	nObjectNode() {}
	nObjectNode(const T& Obj): Object(Obj) {}

	nObjectNode*	GetSucc() const { return (nObjectNode<T>*)nNode::GetSucc(); }
	nObjectNode*	GetPred() const { return (nObjectNode<T>*)nNode::GetPred(); }
	T&				GetObject() { return Object; }
};

#endif
