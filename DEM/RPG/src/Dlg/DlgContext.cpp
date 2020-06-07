#include "DlgContext.h"

#include <Game/Entity.h>
#include <Events/EventServer.h>
#include <Math/Math.h>

namespace Story
{

void CDlgContext::Trigger(bool IsForeground)
{
	if (State == DlgState_Waiting && !IsForeground)
	{
		//advance timer
		//if time has come,
			State = DlgState_InLink;
	}

	while (State == DlgState_InNode || State == DlgState_InLink)
	{
		n_assert(pCurrNode);

		if (State == DlgState_InNode)
		{
			//NodeEnterTime = (float)GameSrv->GetTime();

			// Gather valid links
			ValidLinkIndices.Clear();
			for (UPTR i = 0; i < pCurrNode->Links.GetCount() ; ++i)
			{
				CDlgNode::CLink& Link = pCurrNode->Links[i];
				if (!Link.Condition.IsValid() || Dlg->ScriptObj->RunFunction(Link.Condition.CStr()) == Success)
				{
					ValidLinkIndices.Add(i);
					if (pCurrNode->LinkMode == CDlgNode::Link_Switch) break;
				}
			}

			// Select valid link, if possible
			if (pCurrNode->LinkMode == CDlgNode::Link_Select) LinkIdx = -1;
			else
			{
				// Select random link here because it may be nullptr-link, and UI must
				// know it to display proper text (Continue/End) on the button
				int ValidLinkCount = ValidLinkIndices.GetCount();
				if (ValidLinkCount == 0) LinkIdx = -1;
				else if (ValidLinkCount == 1) LinkIdx = ValidLinkIndices[0];
				else LinkIdx = ValidLinkIndices[Math::RandomU32(0, ValidLinkCount - 1)];
			}

			// Handle node enter externally
			//!!!Looks stupid! Is there more clever way?
			if (IsForeground)
			{
				EventSrv->FireEvent(CStrID("OnForegroundDlgNodeEnter"));
				State = (pCurrNode->Phrase.IsValid() || pCurrNode->LinkMode == CDlgNode::Link_Select) ?
					DlgState_Waiting :
					DlgState_InLink;
			}
			else
			{
				n_assert2(pCurrNode->LinkMode != CDlgNode::Link_Select, "Background dialogues don't support Link_Select!");

				CStrID SpeakerEntity = pCurrNode->SpeakerEntity;
				if (SpeakerEntity == CStrID("$DlgOwner")) SpeakerEntity = DlgOwner;
				else if (SpeakerEntity == CStrID("$PlrSpeaker")) SpeakerEntity = PlrSpeaker;
				Game::PEntity Speaker;// = GameSrv->GetEntityMgr()->GetEntity(SpeakerEntity);
				if (Speaker.IsNullPtr())
					Sys::Error("CDlgContext::Trigger -> speaker entity '%s' not found", SpeakerEntity.CStr());

				Speaker->FireEvent(CStrID("OnDlgNodeEnter"));

				//!!!DBG TMP!
				float DBGTMPTimeToWait = 0.f;
				State = DBGTMPTimeToWait > 0.f ? DlgState_Waiting : DlgState_InLink;
			}
		}

		if (State == DlgState_InLink)
		{
			if (LinkIdx < 0 || LinkIdx > pCurrNode->Links.GetCount()) State = DlgState_Finished;
			else
			{
				CDlgNode::CLink& Link = pCurrNode->Links[LinkIdx];
				UPTR Result = Link.Action.IsValid() ? Dlg->ScriptObj->RunFunction(Link.Action.CStr()) : Success;
				switch (Result)
				{
					case Running: break;
					case Success:
					{
						pCurrNode = Link.pTargetNode;
						State = pCurrNode ? DlgState_InNode : DlgState_Finished;
						break;
					}
					default:
					{
						Sys::Log("CDlgContext::Trigger() > Initiator: '%s'. Scripted action '%s' %s\n",
							Initiator.CStr(), Link.Action.CStr(), Result == Failure ? "failure" : "error");
						State = DlgState_Aborted;
						break;
					}
				}
			}

			if (State == DlgState_Finished || State == DlgState_Aborted)
			{
				Data::PParams P = n_new(Data::CParams(4));
				P->Set(CStrID("Initiator"), Initiator);
				P->Set(CStrID("Target"), Target);
				P->Set(CStrID("IsForeground"), IsForeground);
				P->Set(CStrID("IsAborted"), State == DlgState_Aborted);
				EventSrv->FireEvent(CStrID("OnDlgEnd"), P);
			}
		}
	}
}
//---------------------------------------------------------------------

void CDlgContext::SelectValidLink(UPTR Idx)
{
	n_assert(State == DlgState_Waiting);
	LinkIdx = ValidLinkIndices[Idx];
	State = DlgState_InLink;
}
//---------------------------------------------------------------------

}
