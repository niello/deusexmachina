#ifndef N_HASHNODE_H
#define N_HASHNODE_H
//------------------------------------------------------------------------------
/**
    @class nHashNode
    @ingroup NebulaDataTypes

    @brief A node element in a nHashList.

    (C) 2002 RadonLabs GmbH
*/
#include "kernel/ntypes.h"
#include "util/nstrnode.h"
#include "util/nhashtable.h"

//------------------------------------------------------------------------------
class nHashNode : public nDataNode
{
public:
    /// default constructor
    nHashNode();
    /// constructor with given name
    nHashNode(const char* name);
    /// sets hash table for this node
    void SetHashTable(nHashTable* t);
    /// return next hash node
    nHashNode* GetSucc() const;
    /// return previous hash node
    nHashNode* GetPred() const;
    /// remove this node from list
    void Remove();
    /// get name of the node
    const char* GetName() const;
    /// set name of node
    void SetName(const char* name);

private:
    friend class nHashList;
    nStrNode str_node;
    nHashTable *h_table;
};

//------------------------------------------------------------------------------
/**
*/
inline
nHashNode::nHashNode() :
    str_node((void*)this),
    h_table(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
nHashNode::nHashNode(const char* name) :
    str_node(name, (void*) this),
    h_table(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nHashNode::SetHashTable(nHashTable* t)
{
    // t can be 0!
    this->h_table = t;
}

//------------------------------------------------------------------------------
/**
*/
inline
nHashNode*
nHashNode::GetSucc() const
{
    return (nHashNode*) nNode::GetSucc();
}

//------------------------------------------------------------------------------
/**
*/
inline
nHashNode*
nHashNode::GetPred() const
{
    return (nHashNode *) nNode::GetPred();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nHashNode::Remove()
{
    this->str_node.Remove();
    nNode::Remove();
    this->h_table = 0;
}

//------------------------------------------------------------------------------
/**
*/
inline
const char*
nHashNode::GetName() const
{
    return this->str_node.GetName();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nHashNode::SetName(const char* name)
{
    if (this->IsLinked())
    {
        n_assert(this->h_table);
        this->str_node.Remove();
        this->str_node.SetName(name);
        this->h_table->Add(&(this->str_node));
    }
    else
    {
        this->str_node.SetName(name);
    }
}

//------------------------------------------------------------------------------
#endif
