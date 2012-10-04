#include "DlgNodePhrase.h"

#include "DlgSystem.h"
#include "DlgLink.h"
#include <Game/GameServer.h>
#include <Events/EventManager.h>

namespace Story
{
ImplementRTTI(Story::CDlgNodePhrase, Story::CDlgNode);
ImplementFactory(Story::CDlgNodePhrase);

void CDlgNodePhrase::OnEnter(CActiveDlg& Dlg)
{
	DlgSys->SayPhrase(SpeakerEntity, Phrase, Dlg);
}
//---------------------------------------------------------------------

CDlgNode* CDlgNodePhrase::Trigger(CActiveDlg& Dlg)
{
	while (Dlg.IsCheckingConditions && Dlg.LinkIdx < Links.Size())
	{
		EExecStatus Status = Links[Dlg.LinkIdx]->Validate(Dlg);
		if (Status == Success)
		{
			if (Links[Dlg.LinkIdx]->pTargetNode)
			{
				Dlg.IsCheckingConditions = false;
				EventMgr->FireEvent(CStrID("OnDlgContinueAvailable"));
			}
			break;
		}
		else if (Status == Running) return this;
		else Dlg.LinkIdx++;
	}

	if (Dlg.IsCheckingConditions)
	{
		Dlg.IsCheckingConditions = false;
		Dlg.Continued = false;
		EventMgr->FireEvent(CStrID("OnDlgEndAvailable"));
	}

	if (!Dlg.Continued && (Timeout < 0.f || Dlg.NodeEnterTime + Timeout > GameSrv->GetTime())) return this;

	return (Dlg.LinkIdx == Links.Size()) ? NULL : Links[Dlg.LinkIdx]->DoTransition(Dlg);
}
//---------------------------------------------------------------------

} //namespace AI