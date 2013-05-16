#include "DlgNode.h"

#include "DialogueManager.h"
#include "DlgLink.h"

namespace Story
{
__ImplementClass(Story::CDlgNode, 'DLGN', Core::CRefCounted);

CDlgNode* CDlgNode::Trigger(CActiveDlg& Dlg)
{
	while (Dlg.IsCheckingConditions && Dlg.LinkIdx < Links.GetCount())
	{
		EExecStatus Status = Links[Dlg.LinkIdx]->Validate(Dlg);
		if (Status == Success) Dlg.IsCheckingConditions = false;
		else if (Status == Running) return this;
		else Dlg.LinkIdx++;
	}

	return (Dlg.LinkIdx == Links.GetCount()) ? NULL : Links[Dlg.LinkIdx]->DoTransition(Dlg);
}
//---------------------------------------------------------------------

} //namespace AI