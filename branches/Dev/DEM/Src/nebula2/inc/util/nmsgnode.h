#ifndef N_MSGNODE_H
#define N_MSGNODE_H
//------------------------------------------------------------------------------
/**
    @class nMsgNode
    @ingroup NebulaThreadingSupport
    @ingroup NebulaDataTypes

    @brief A doubly linked list node which can carry a msg.

    (C) 2002 RadonLabs GmbH
*/
#include <string.h>
#include "util/nnode.h"

//------------------------------------------------------------------------------
class nMsgNode : public nNode
{
public:
    /// constructor
    nMsgNode(void* buf, int size, int client);
    /// destructor
    ~nMsgNode();
    /// get pointer to message
    void* GetMsgPtr() const;
    /// get size of message
    int GetMsgSize() const;

	int GetClientID() const { return clientId; }

private:
    char* msgBuf;
    int msgSize;
	int clientId;
};

//------------------------------------------------------------------------------
/**
*/
inline
nMsgNode::nMsgNode(void* buf, int size, int client)
{
    n_assert(buf);
    n_assert(size > 0);
    this->msgBuf  = (char*)n_malloc(size);
    this->msgSize = size;
    n_assert(this->msgBuf);
    memcpy(this->msgBuf, buf, size);
	clientId = client;
}

//------------------------------------------------------------------------------
/**
*/
inline
nMsgNode::~nMsgNode()
{
    n_free(this->msgBuf);
}

//------------------------------------------------------------------------------
/**
*/
inline
void*
nMsgNode::GetMsgPtr() const
{
    return this->msgBuf;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMsgNode::GetMsgSize() const
{
    return this->msgSize;
}

//------------------------------------------------------------------------------
#endif

