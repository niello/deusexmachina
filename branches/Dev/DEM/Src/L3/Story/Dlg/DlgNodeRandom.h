#pragma once
#ifndef __IPG_DLG_NODE_RANDOM_H__
#define __IPG_DLG_NODE_RANDOM_H__

#include "DlgNode.h"

// Dialogue random node. Selects random valid link to follow.

namespace Story
{

class CDlgNodeRandom: public CDlgNode
{
	DeclareRTTI;
	DeclareFactory(CDlgNodeRandom);

public:

	virtual void		OnEnter(CActiveDlg& Dlg);
	virtual CDlgNode*	Trigger(CActiveDlg& Dlg);
};

RegisterFactory(CDlgNodeRandom);

}

#endif
