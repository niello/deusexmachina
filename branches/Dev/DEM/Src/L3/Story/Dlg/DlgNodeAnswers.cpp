#include "DlgNodeAnswers.h"

#include "DlgSystem.h"
#include "DlgLink.h"
#include "DlgNodePhrase.h"
#include <Events/EventManager.h>

namespace Story
{
ImplementRTTI(Story::CDlgNodeAnswers, Story::CDlgNode);
ImplementFactory(Story::CDlgNodeAnswers);

void CDlgNodeAnswers::OnEnter(CActiveDlg& Dlg)
{
	DlgSys->SayPhrase(SpeakerEntity, Phrase, Dlg);
}
//---------------------------------------------------------------------

CDlgNode* CDlgNodeAnswers::Trigger(CActiveDlg& Dlg)
{
#ifdef _DEBUG
	n_assert2(DlgSys->IsDialogueForeground(Dlg) && Dlg.PlrSpeaker.IsValid(),
		"Answer node can be used only in a foreground dialogue with player");
#endif

	if (Dlg.IsCheckingConditions)
	{
		while (Dlg.LinkIdx < Links.Size())
		{
			EExecStatus Status = Links[Dlg.LinkIdx]->Validate(Dlg);
			if (Status == Success)
			{
				n_assert2(Links[Dlg.LinkIdx]->pTargetNode->IsA(CDlgNodePhrase::RTTI), "Answer dlg node should contain ONLY phrase nodes!");
				
				Dlg.ValidLinkIndices.Append(Dlg.LinkIdx);

				//???send link index too?
				PParams P = n_new(CParams);
				P->Set(CStrID("Phrase"), (PVOID)((CDlgNodePhrase*)Links[Dlg.LinkIdx]->pTargetNode)->Phrase.Get());
				EventMgr->FireEvent(CStrID("OnDlgAnswerVariantAdded"), P);
			}
			else if (Status == Running) return this;
			Dlg.LinkIdx++;
		}

		EventMgr->FireEvent(CStrID((Dlg.ValidLinkIndices.Size() > 0) ? "OnDlgAnswersAvailable" : "OnDlgEndAvailable"));
		Dlg.IsCheckingConditions = false;
	}

	if (!Dlg.Continued) return this;

	return (Dlg.LinkIdx == Links.Size()) ? NULL : Links[Dlg.LinkIdx]->DoTransition(Dlg);
}
//---------------------------------------------------------------------

} //namespace AI