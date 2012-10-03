#ifndef N_STRNODE_H
#define N_STRNODE_H
//------------------------------------------------------------------------------
/**
    @class nStrNode
    @ingroup NebulaDataTypes

    @brief A node in a nStrList.

    (C) 2002 RadonLabs GmbH
*/
#include "util/ndatanode.h"
#include "util/nstring.h"

//------------------------------------------------------------------------------
class nStrNode: public nDataNode
{
public:
    /// default constructor
	nStrNode() {}
    /// constructor providing custom data pointer
    nStrNode(void* _ptr): nDataNode(_ptr) {}
    /// constructor providing node name
	nStrNode(const char* str): name(str) {}
    /// constructor providing custom data pointer and node name
	nStrNode(const char* str, void* _ptr): nDataNode(_ptr), name(str) {}
    /// set the name of this node
    void SetName(const char* str);
    /// get the name of this node
    const char* GetName() const;
    /// get next node in list
    nStrNode* GetSucc() const;
    /// get previous node in list
    nStrNode* GetPred() const;

private:
    nString name;
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nStrNode::SetName(const char* str)
{
    this->name.Set(str);
}

//------------------------------------------------------------------------------
/**
*/
inline
const char*
nStrNode::GetName() const
{
    return this->name.IsValid() ? this->name.Get() : 0;
}

//------------------------------------------------------------------------------
/**
*/
inline
nStrNode*
nStrNode::GetSucc() const
{
    return (nStrNode*)nNode::GetSucc();
}

//------------------------------------------------------------------------------
/**
*/
inline
nStrNode*
nStrNode::GetPred() const
{
    return (nStrNode*)nNode::GetPred();
}

//------------------------------------------------------------------------------
#endif
