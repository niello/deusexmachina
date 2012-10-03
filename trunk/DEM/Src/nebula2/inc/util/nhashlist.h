#ifndef N_HASHLIST_H
#define N_HASHLIST_H
//------------------------------------------------------------------------------
/**
    @class nHashList
    @ingroup NebulaDataTypes

    @brief A doubly linked list of named nodes with fast hashtable based search.

    (C) 2002 RadonLabs GmbH
*/
#include "kernel/ntypes.h"
#include "util/nhashnode.h"

//------------------------------------------------------------------------------
class nHashList : public nList
{
public:
    // default constructor
    nHashList();
    /// constructor with given hash table size
    nHashList(int hashsize);
    /// get first node
    nHashNode* GetHead() const;
    /// get last node
    nHashNode* GetTail() const;
    /// add node to beginning of list
    void AddHead(nHashNode* n);
    /// add node to end of list
    void AddTail(nHashNode* n);
    /// remove first node
    nHashNode* RemHead();
    /// remove last node
    nHashNode* RemTail();
    /// search node by name
    nHashNode* Find(const char* name) const;

private:
    enum
    {
        N_DEFAULT_HASHSIZE = 16,
    };
    nHashTable h_table;
};

//------------------------------------------------------------------------------
/**
*/
inline
nHashList::nHashList() :
    h_table(N_DEFAULT_HASHSIZE)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
nHashList::nHashList(int hashsize) :
    h_table(hashsize)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
nHashNode*
nHashList::GetHead() const
{
    return (nHashNode*)nList::GetHead();
}

//------------------------------------------------------------------------------
/**
*/
inline
nHashNode*
nHashList::GetTail() const
{
    return (nHashNode*)nList::GetTail();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nHashList::AddHead(nHashNode* n)
{
    n_assert(n);
    n->SetHashTable(&(this->h_table));
    this->h_table.Add(&(n->str_node));
    nList::AddHead((nNode*)n);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nHashList::AddTail(nHashNode* n)
{
    n_assert(n);
    n->SetHashTable(&(this->h_table));
    this->h_table.Add(&(n->str_node));
    nList::AddTail((nNode*)n);
}

//------------------------------------------------------------------------------
/**
*/
inline
nHashNode*
nHashList::RemHead()
{
    nHashNode* n = (nHashNode*)nList::RemHead();
    if (n)
    {
        n->str_node.Remove();
        n->SetHashTable(0);
    }
    return n;
}

//------------------------------------------------------------------------------
/**
*/
inline
nHashNode*
nHashList::RemTail()
{
    nHashNode* n = (nHashNode*)nList::RemTail();
    if (n)
    {
        n->str_node.Remove();
        n->SetHashTable(0);
    }
    return n;
}

//------------------------------------------------------------------------------
/**
*/
inline
nHashNode*
nHashList::Find(const char* name) const
{
    nStrNode* sn = this->h_table.Find(name);
    if (sn)
    {
        return (nHashNode*)sn->GetPtr();
    }
    return 0;
}

//------------------------------------------------------------------------------
#endif
