#pragma once
#ifndef __DEM_L3_DLG_CONTEXT_H__
#define __DEM_L3_DLG_CONTEXT_H__

#include <Dlg/DlgGraph.h>

// Dialogue context is an instance that stores execution state associated with a dialogue graph

namespace Story
{

class CDlgContext //???struct?
{
public:

	PDlgGraph	Dlg;

	CStrID		Initiator;
	CStrID		Target;
	CStrID		DlgOwner;
	CStrID		PlrSpeaker;

	CDlgNode*	pCurrNode;
	//float		NodeEnterTime;
	int			LinkIdx;
	CArray<int>	ValidLinkIndices;	// For nodes with delayed link selection, like answer nodes

	CDlgContext(): pCurrNode(NULL) {}
};

}

#endif