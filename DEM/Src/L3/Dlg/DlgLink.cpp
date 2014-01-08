#include "DlgLink.h"

#include "DialogueManager.h"

namespace Story
{

DWORD CDlgLink::Validate(CActiveDlg& Dlg)
{
	return Condition.IsValid() ? Dlg.Dlg->ScriptObj->RunFunction(Condition.CStr()) : Success;
}
//---------------------------------------------------------------------

CDlgNode* CDlgLink::DoTransition(CActiveDlg& Dlg)
{
	if (Action.IsEmpty()) return pTargetNode;
	switch (Dlg.Dlg->ScriptObj->RunFunction(Action.CStr()))
	{
		case Success:	return pTargetNode;
		case Running:	return Dlg.pCurrNode;
		default:		return NULL;
	}
}
//---------------------------------------------------------------------

} //namespace AI