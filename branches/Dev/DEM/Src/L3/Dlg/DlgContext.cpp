#include "DlgContext.h"

#include <Dlg/DialogueManager.h>

namespace Story
{

void CDlgContext::HandleNode()
{
	n_assert(pCurrNode);
	//NodeEnterTime = (float)GameSrv->GetTime();

	ValidLinkIndices.Clear();
	for (int i = 0; i < pCurrNode->Links.GetCount() ; ++i)
	{
		CDlgNode::CLink& Link = pCurrNode->Links[i];
		if (!Link.Condition.IsValid() || Dlg->ScriptObj->RunFunction(Link.Condition.CStr()) == Success)
		{
			ValidLinkIndices.Add(i);
			if (pCurrNode->LinkMode == CDlgNode::Link_Switch) break;
		}
	}

	if (pCurrNode->LinkMode == CDlgNode::Link_Select) LinkIdx = -1;
	else
	{
		// Select random link here because it may be NULL-link, and UI must
		// know it to display proper text (Continue/End) on the button
		int ValidLinkCount = ValidLinkIndices.GetCount();
		if (ValidLinkCount == 0) LinkIdx = -1;
		else if (ValidLinkCount == 1) LinkIdx = ValidLinkIndices[0];
		else LinkIdx = ValidLinkIndices[n_rand_int(0, ValidLinkCount - 1)];
	}

	State = DlgState_Waiting;
}
//---------------------------------------------------------------------

void CDlgContext::HandleLink()
{
	if (LinkIdx < 0 || LinkIdx > pCurrNode->Links.GetCount())
	{
		State = DlgState_Finished;
		return;
	}

	CDlgNode::CLink& Link = pCurrNode->Links[LinkIdx];
	if (Link.Action.IsEmpty())
	{
		pCurrNode = Link.pTargetNode;
		State = pCurrNode ? DlgState_InNode : DlgState_Finished;
		return;
	}

	switch (Dlg->ScriptObj->RunFunction(Link.Action.CStr()))
	{
		case Running:	return;
		case Success:
		{
			pCurrNode = Link.pTargetNode;
			State = pCurrNode ? DlgState_InNode : DlgState_Finished;
			return;
		}
		default:
		{
			State = DlgState_Aborted;
			return; // Action failure or error //???what to do on Failure?
		}
	}
}
//---------------------------------------------------------------------

void CDlgContext::SelectValidLink(int Idx)
{
	n_assert(State == DlgState_InNode);
	LinkIdx = ValidLinkIndices[Idx];
	State = DlgState_InLink;
}
//---------------------------------------------------------------------

}