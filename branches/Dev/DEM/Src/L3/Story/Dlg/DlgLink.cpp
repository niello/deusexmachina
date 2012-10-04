#include "DlgLink.h"

#include "DlgSystem.h"

namespace Story
{

EExecStatus CDlgLink::Validate(CActiveDlg& Dlg)
{
	return Condition.IsValid() ? Dlg.Dlg->ScriptObj->RunFunction(Condition.Get()) : Success;
}
//---------------------------------------------------------------------

CDlgNode* CDlgLink::DoTransition(CActiveDlg& Dlg)
{
	if (Action.IsEmpty()) return pTargetNode;
	switch (Dlg.Dlg->ScriptObj->RunFunction(Action.Get()))
	{
		case Success:	return pTargetNode;
		case Running:	return Dlg.pCurrNode;
		default:		return NULL;
	}
}
//---------------------------------------------------------------------

} //namespace AI