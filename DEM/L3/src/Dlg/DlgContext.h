#pragma once
#ifndef __DEM_L3_DLG_CONTEXT_H__
#define __DEM_L3_DLG_CONTEXT_H__

#include <Dlg/DlgGraph.h>

// Dialogue context is an instance that stores execution state associated with a dialogue graph,
// and implements FSM traversal of that dialogue graph.

namespace Story
{

enum EDlgState
{
	DlgState_None = 0,		// Does not exist
	DlgState_Requested = 1,	// Requested, not accepted by target
	DlgState_InNode = 2,	// Accepted, current node must be processed
	DlgState_Waiting = 3,	// Current node was processed, wait for a time, UI or other response
	DlgState_InLink = 4,	// Response received, follow selected link
	DlgState_Finished = 5,	// Exited node with no valid links or with a link to NULL
	DlgState_Aborted = 6	// Rejected or failed to execute script (or aborted by user?)
};

class CDlgContext
{
public:

	PDlgGraph	Dlg;
	EDlgState	State;

	CStrID		Initiator;
	CStrID		Target;
	CStrID		DlgOwner;
	CStrID		PlrSpeaker;

	CDlgNode*	pCurrNode;
	//float		NodeEnterTime;
	UPTR		LinkIdx;
	CArray<int>	ValidLinkIndices;	// For nodes with delayed link selection, like answer nodes

	CDlgContext(): pCurrNode(NULL) {}

	void Trigger(bool IsForeground);
	void SelectValidLink(UPTR Idx);
};

}

#endif