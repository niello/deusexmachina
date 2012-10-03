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

template <class TYPE>
class nObjectNode: public nNode
{
public:

	TYPE Object;

	nObjectNode() {}
	nObjectNode(const TYPE& object): Object(object) {}

	nObjectNode* GetSucc() const { return (nObjectNode<TYPE>*)nNode::GetSucc(); }
	nObjectNode* GetPred() const { return (nObjectNode<TYPE>*)nNode::GetPred(); }
};

#endif
