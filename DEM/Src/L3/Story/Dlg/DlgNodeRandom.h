#pragma once
#ifndef __DEM_L3_DLG_NODE_RANDOM_H__
#define __DEM_L3_DLG_NODE_RANDOM_H__

#include "DlgNode.h"

// Dialogue random node. Selects random valid link to follow.

namespace Story
{

class CDlgNodeRandom: public CDlgNode
{
	__DeclareClass(CDlgNodeRandom);

public:

	virtual void		OnEnter(CActiveDlg& Dlg);
	virtual CDlgNode*	Trigger(CActiveDlg& Dlg);
};

__RegisterClassInFactory(CDlgNodeRandom);

}

#endif
