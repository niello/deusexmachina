#include "DlgNodeRandom.h"

#include "DialogueManager.h"
#include "DlgLink.h"

namespace Story
{
__ImplementClassNoFactory(Story::CDlgNodeRandom, Story::CDlgNode);
__ImplementClass(Story::CDlgNodeRandom);

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