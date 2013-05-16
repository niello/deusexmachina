#include "DlgNodeRandom.h"

#include "DialogueManager.h"
#include "DlgLink.h"

namespace Story
{
__ImplementClass(Story::CDlgNodeRandom, 'DLNR', Story::CDlgNode);

void CDlgNodeRandom::OnEnter(CActiveDlg& Dlg)
{
}
//---------------------------------------------------------------------

CDlgNode* CDlgNodeRandom::Trigger(CActiveDlg& Dlg)
{
	return Links[Dlg.LinkIdx]->DoTransition(Dlg);
}
//---------------------------------------------------------------------

} //namespace AI