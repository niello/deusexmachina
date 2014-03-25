#include "DlgNode.h"

#include <Dlg/DialogueManager.h>

namespace Story
{
__ImplementClass(Story::CDlgNode, 'DLGN', Core::CRefCounted);

//???from where must be called?!
void CDlgNode::OnEnter(CDlgContext& Context)
{
	Context.pCurrNode = this;
	//Context.NodeEnterTime = (float)GameSrv->GetTime();

	Context.ValidLinkIndices.Clear();
	for (int i = 0; i < Links.GetCount() ; ++i)
	{
		CLink& Link = Links[i];
		if (!Link.Condition.IsValid() || Context.Dlg->ScriptObj->RunFunction(Link.Condition.CStr()) == Success)
		{
			Context.ValidLinkIndices.Add(i);
			if (LinkMode == Link_Switch) break;
		}
	}

	if (LinkMode == Link_Select) Context.LinkIdx = -1;
	else
	{
		// Select random link here because it may be NULL-link, and UI must
		// know it to display proper text (Continue/End) on the button
		int ValidLinkCount = Context.ValidLinkIndices.GetCount();
		if (ValidLinkCount == 0) Context.LinkIdx = -1;
		else if (ValidLinkCount == 1) Context.LinkIdx = Context.ValidLinkIndices[0];
		else Context.LinkIdx = n_rand_int(0, ValidLinkCount - 1);
	}

	DlgMgr->HandleNode(Context);
}
//---------------------------------------------------------------------

CDlgNode* CDlgNode::FollowLink(CDlgContext& Context)
{
	CLink& Link = Links[Context.LinkIdx];
	if (Link.Action.IsEmpty()) return Link.pTargetNode;
	switch (Context.Dlg->ScriptObj->RunFunction(Link.Action.CStr()))
	{
		case Success:	return Link.pTargetNode;
		case Running:	return Context.pCurrNode;
		default:		return NULL;
	}
}
//---------------------------------------------------------------------

}