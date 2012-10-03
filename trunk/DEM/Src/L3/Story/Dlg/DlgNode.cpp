#include "DlgNode.h"

#include "DlgSystem.h"
#include "DlgLink.h"

namespace Story
{
ImplementRTTI(Story::CDlgNode, Core::CRefCounted);
ImplementFactory(Story::CDlgNode);

CDlgNode* CDlgNode::Trigger(CActiveDlg& Dlg)
{
	while (Dlg.IsCheckingConditions && Dlg.LinkIdx < Links.Size())
	{
		EExecStatus Status = Links[Dlg.LinkIdx]->Validate(Dlg);
		if (Status == Success) Dlg.IsCheckingConditions = false;
		else if (Status == Running) return this;
		else Dlg.LinkIdx++;
	}

	return (Dlg.LinkIdx == Links.Size()) ? NULL : Links[Dlg.LinkIdx]->DoTransition(Dlg);
}
//---------------------------------------------------------------------

} //namespace AI