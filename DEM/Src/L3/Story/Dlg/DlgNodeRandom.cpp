#include "DlgNodeRandom.h"

#include "DlgSystem.h"
#include "DlgLink.h"

namespace Story
{
ImplementRTTI(Story::CDlgNodeRandom, Story::CDlgNode);
ImplementFactory(Story::CDlgNodeRandom);

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