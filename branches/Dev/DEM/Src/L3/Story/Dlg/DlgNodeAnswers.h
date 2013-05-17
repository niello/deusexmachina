#pragma once
#ifndef __DEM_L3_DLG_NODE_ANSWERS_H__
#define __DEM_L3_DLG_NODE_ANSWERS_H__

#include "DlgNode.h"
#include <Data/StringID.h>

// Dialogue answers node. It uses attached phrase nodes as answer variants and waits for player choice.
// Don't attach any other types of nodes to it.

namespace Story
{

class CDlgNodeAnswers: public CDlgNode
{
	__DeclareClass(CDlgNodeAnswers);

public:

	CStrID				SpeakerEntity; //!!!special cases Starter & Support!
	nString				Phrase;
	//???default answer index or special OnAbort action? or can end dlg only by answering?

	virtual void		OnEnter(CActiveDlg& Dlg);
	virtual CDlgNode*	Trigger(CActiveDlg& Dlg);
};

__RegisterClassInFactory(CDlgNodeAnswers);

}

#endif
