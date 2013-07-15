#pragma once
#ifndef __IPG_DLG_LINK_H__
#define __IPG_DLG_LINK_H__

#include <Core/RefCounted.h>
#include <StdDEM.h>

// Link between dialogue nodes. Contains activation conditions, actions and target node

namespace Story
{
class CDlgNode;
class CActiveDlg;

class CDlgLink: public Core::CRefCounted //???or declare as struct inside node?
{
public:

	CString		Condition;	// Scripted function name
	CString		Action;		// Scripted function name
	CDlgNode*	pTargetNode;

	CDlgLink(): pTargetNode(NULL) {}

	EExecStatus Validate(CActiveDlg& Dlg);
	CDlgNode*	DoTransition(CActiveDlg& Dlg);
};

}

#endif
