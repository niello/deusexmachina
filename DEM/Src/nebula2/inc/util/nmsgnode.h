#ifndef N_MSGNODE_H
#define N_MSGNODE_H

#include <string.h>
#include "util/nnode.h"

// A doubly linked list node which can carry a msg. Used for a threading support.
// (C) 2002 RadonLabs GmbH

class nMsgNode: public nNode
{
private:

	char*	pMsgBuf;
	int		MsgSize;
	int		ClientID;

public:

	nMsgNode(void* pBuffer, int Size, int Client);
	~nMsgNode() { n_free(pMsgBuf); }

	void*	GetMsgPtr() const { return pMsgBuf; }
	int		GetMsgSize() const { return MsgSize; }
	int		GetClientID() const { return ClientID; }
};

inline nMsgNode::nMsgNode(void* pBuffer, int Size, int Client): MsgSize(Size), ClientID(Client)
{
	n_assert(pBuffer && Size > 0);
	pMsgBuf = (char*)n_malloc(Size);
	n_assert(pMsgBuf);
	memcpy(pMsgBuf, pBuffer, Size);
}
//---------------------------------------------------------------------

#endif

