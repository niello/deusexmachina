#pragma once
#ifndef __DEM_L3_DLG_NODE_PHRASE_H__
#define __DEM_L3_DLG_NODE_PHRASE_H__

#include "DlgNode.h"
#include <Data/StringID.h>

// Dialogue phrase node. It simply outputs a phrase and waits for continue. Potentially can have timeout.

namespace Story
{
class CDlgLink;

class CDlgNodePhrase: public CDlgNode
{
	__DeclareClass(CDlgNodePhrase);

public:

	CStrID	SpeakerEntity;
	nString	Phrase;
	float	Timeout;

	CDlgNodePhrase(): Timeout(-1.f) {}

	virtual void		OnEnter(CActiveDlg& Dlg);
	virtual CDlgNode*	Trigger(CActiveDlg& Dlg);
};

}

#endif
