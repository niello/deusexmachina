#include "DialogueManager.h"

#include "Dialogue.h"
#include "DlgNodePhrase.h"
#include "DlgNodeAnswers.h"
#include "DlgNodeRandom.h"
#include "DlgLink.h"
#include <Chr/Prop/PropTalking.h>
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <Data/DataServer.h>
#include <IO/IOServer.h>
#include <Scripting/ScriptServer.h>
#include <Events/EventManager.h>
#include <Game/EntityManager.h>
#include <Game/GameServer.h>

namespace Story
{
__ImplementClassNoFactory(Story::CDialogueManager, Core::CRefCounted);
__ImplementSingleton(Story::CDialogueManager);

void CActiveDlg::EnterNode(CDlgNode* pNewNode)
{
	n_assert(pNewNode);
	NodeEnterTime = (float)GameSrv->GetTime();
	Continued = false;
	IsCheckingConditions = true;
	LinkIdx = 0;
	ValidLinkIndices.Clear();
	pCurrNode = pNewNode;
	pCurrNode->OnEnter(*this);
}
//---------------------------------------------------------------------

CDialogueManager::CDialogueManager()
{
    __ConstructSingleton;
	IOSrv->SetAssign("dlg", "game:dlg");
}
//---------------------------------------------------------------------

CDialogueManager::~CDialogueManager()
{
    __DestructSingleton;
}
//---------------------------------------------------------------------

PDialogue CDialogueManager::CreateDialogue(const CParams& Params, const nString& Name)
{
	nDictionary<CStrID, Ptr<CDlgNode>> LoadedNodes;

	//!!!can store dialog as node (not node ptr) array, lesser allocations needed!

	PDialogue Dlg = n_new(CDialogue);

	const PParams& Nodes = Params.Get<PParams>(CStrID("Nodes"));
	for (int i = 0; i < Nodes->GetCount(); i++)
	{
		const CParam& Node = (*Nodes)[i];
		const PParams& NodeData = Node.GetValue<PParams>();
		int Type = NodeData->Get<int>(CStrID("Type"));
		Ptr<CDlgNode> NewNode;
		switch (Type)
		{
			case DLG_NODE_EMPTY:
				{
					NewNode = n_new(CDlgNode);
					break;
				}
			case DLG_NODE_PHRASE:
				{
					CDlgNodePhrase* pNewNode = n_new(CDlgNodePhrase);
					pNewNode->SpeakerEntity = CStrID(NodeData->Get<nString>(CStrID("Speaker")).CStr());
					pNewNode->Phrase = NodeData->Get<nString>(CStrID("Phrase"));
					NewNode = pNewNode;
					break;
				}
			case DLG_NODE_ANSWERS:
				{
					CDlgNodeAnswers* pNewNode = n_new(CDlgNodeAnswers);
					pNewNode->SpeakerEntity = CStrID(NodeData->Get<nString>(CStrID("Speaker")).CStr());
					pNewNode->Phrase = NodeData->Get<nString>(CStrID("Phrase"));
					NewNode = pNewNode;
					break;
				}
			case DLG_NODE_RANDOM:
				{
					NewNode = n_new(CDlgNodeRandom);
					break;
				}
			default: n_error("CDialogueManager::CreateDialogue: Unknown Dlg Node type!");
		}
		n_assert(NewNode.IsValid());
		LoadedNodes.Add(Node.GetName(), NewNode);
		Dlg->Nodes.Append(NewNode);
	}
	
	int	Idx = Params.IndexOf(CStrID("StartNode"));
	if (Idx == INVALID_INDEX) Dlg->StartNode = n_new(CDlgNode);
	else Dlg->StartNode = LoadedNodes[CStrID(Params.Get(Idx).GetValue<nString>().CStr())];
	n_assert(Dlg->StartNode);
	Dlg->Nodes.Append(Dlg->StartNode);

	bool UsesScript = false;

	const CDataArray& Links = *(Params.Get<PDataArray>(CStrID("Links")));
	for (int i = 0; i < Links.GetCount(); i++)
	{
		const CDataArray& Link = *(Links.Get(i).GetValue<PDataArray>());

		Ptr<CDlgLink> NewLink = n_new(CDlgLink);

		const nString& PrmFrom = Link.Get(0).GetValue<nString>();
		Ptr<CDlgNode> From = PrmFrom.IsEmpty() ? Dlg->StartNode : LoadedNodes[CStrID(PrmFrom.CStr())];
		n_assert(From.IsValid());

		if (Link.GetCount() > 1)
		{
			const nString& PrmTo = Link.Get(1).GetValue<nString>();
			if (PrmTo.IsValid()) NewLink->pTargetNode = LoadedNodes[CStrID(PrmTo.CStr())];
		}

		if (Link.GetCount() > 2)
		{
			NewLink->Condition = Link.Get(2).GetValue<nString>();
			if (!UsesScript && NewLink->Condition.IsValid()) UsesScript = true;
		}

		if (Link.GetCount() > 3)
		{
			NewLink->Action = Link.Get(3).GetValue<nString>();
			if (!UsesScript && NewLink->Action.IsValid()) UsesScript = true;
		}

		From->Links.Append(NewLink);
	}

	if (UsesScript)
	{
		Idx = Params.IndexOf(CStrID("ScriptFile"));
		if (Idx == INVALID_INDEX)
		{
			Dlg->ScriptFile = "dlg:";
			Dlg->ScriptFile += Name;
			Dlg->ScriptFile += ".lua";
		}
		else Dlg->ScriptFile = Params.Get(Idx).GetValue<nString>();
	}

	return Dlg;
}
//---------------------------------------------------------------------

PDialogue CDialogueManager::GetDialogue(const nString& Name) //???CStrID identifier?
{
	CStrID SID = CStrID(Name.CStr());
	int Idx = DlgRegistry.FindIndex(SID);
	if (Idx > -1) return DlgRegistry.ValueAt(Idx);
	else
	{
		PParams Desc = DataSrv->LoadPRM(nString("dlg:") + Name + ".prm", false);
		if (Desc.IsValid())
		{
			PDialogue NewDlg = CreateDialogue(*Desc, Name);
			DlgRegistry.Add(SID, NewDlg);
			return NewDlg;
		}
		else return PDialogue();
	}
}
//---------------------------------------------------------------------

void CDialogueManager::StartDialogue(CEntity* pTarget, CEntity* pInitiator, bool Foreground)
{
	if (Foreground && IsDialogueActive())
	{
		n_printf("Error, Dlg: Trying to start new foreground dialogue when other is active.");
		return;
	}

	n_assert(pTarget && pInitiator);

	//!!!USE PARTY MGR! FactionMgr->GetParty()->GetLeaderID() or smth.
	bool TargetIsPlr = pTarget->GetUID() == "GG";
	bool InitiatorIsPlr = pInitiator->GetUID() == "GG";

	CActiveDlg NewDlg;

	if (TargetIsPlr == InitiatorIsPlr) // Plr-Plr & NPC-NPC
	{
		NewDlg.Dlg = pTarget->GetProperty<Prop::CPropTalking>()->GetDialogue();
		if (NewDlg.Dlg.IsValid())
		{
			NewDlg.DlgOwner = pTarget->GetUID();
			if (InitiatorIsPlr) NewDlg.PlrSpeaker = pInitiator->GetUID();
		}
		else
		{
			NewDlg.Dlg = pInitiator->GetProperty<Prop::CPropTalking>()->GetDialogue();
			if (NewDlg.Dlg.IsValid())
			{
				NewDlg.DlgOwner = pInitiator->GetUID();
				if (TargetIsPlr) NewDlg.PlrSpeaker = pTarget->GetUID();
			}
		}
	}
	else // NPC-Plr & Plr-NPC
	{
		CEntity* pNPC = (TargetIsPlr) ? pInitiator : pTarget;
		CEntity* pPlr = (TargetIsPlr) ? pTarget : pInitiator;
		NewDlg.Dlg = pNPC->GetProperty<Prop::CPropTalking>()->GetDialogue();
		if (NewDlg.Dlg.IsValid())
		{
			NewDlg.DlgOwner = pNPC->GetUID();
			NewDlg.PlrSpeaker = pPlr->GetUID();
		}
		else //???need?
		{
			NewDlg.Dlg = pPlr->GetProperty<Prop::CPropTalking>()->GetDialogue();
			if (NewDlg.Dlg.IsValid())
			{
				NewDlg.DlgOwner = pPlr->GetUID();
				NewDlg.PlrSpeaker = pNPC->GetUID();
			}
		}
	}

	if (!NewDlg.Dlg.IsValid()) return;

	if (NewDlg.Dlg->ScriptFile.IsValid() && !NewDlg.Dlg->ScriptObj.IsValid())
	{
		nString ScriptName;
		ScriptName.Format("Dlg_%x", NewDlg.Dlg.GetUnsafe()); //???store CStrID in Dlg & use it here?
		NewDlg.Dlg->ScriptObj = n_new(CScriptObject(ScriptName.CStr(), "Dialogues"));
		NewDlg.Dlg->ScriptObj->Init(); // No special class
		if (NewDlg.Dlg->ScriptObj->LoadScriptFile(NewDlg.Dlg->ScriptFile) != Success)
			n_printf("Error loading script \"%s\" for dialogue", NewDlg.Dlg->ScriptFile.CStr());
	}

	if (NewDlg.Dlg->ScriptObj.IsValid()) NewDlg.Dlg->ScriptObj->RunFunction("OnStart");

	if (Foreground)
	{
		ForegroundDlg = NewDlg;

		if (NewDlg.PlrSpeaker.IsValid())
			EntityMgr->GetEntity(NewDlg.PlrSpeaker)->FireEvent(CStrID("DisableInput"));

		SUBSCRIBE_PEVENT(OnDlgAnswersBegin, CDialogueManager, OnDlgAnswersBegin);

		EventMgr->FireEvent(CStrID("OnDlgStart"));

		ForegroundDlg.EnterNode(ForegroundDlg.Dlg->StartNode);
	}
	else
	{
		BackgroundDlgs.Append(NewDlg);
		BackgroundDlgs.Back().EnterNode(BackgroundDlgs.Back().Dlg->StartNode);
	}
}
//---------------------------------------------------------------------

void CDialogueManager::Trigger()
{
	if (IsDialogueActive())
	{
		CDlgNode* pNewNode = ForegroundDlg.Trigger();
		while (pNewNode != ForegroundDlg.pCurrNode)
		{
			if (pNewNode)
			{
				bool Continue = ForegroundDlg.pCurrNode->IsA(CDlgNodeAnswers::RTTI);
				ForegroundDlg.EnterNode(pNewNode);
				if (Continue) ForegroundDlg.Continued = true;
				EventMgr->FireEvent(CStrID("OnDlgNoCmdAvailable"));
				pNewNode = ForegroundDlg.Trigger();
			}
			else
			{
				n_assert2(!ForegroundDlg.ValidLinkIndices.GetCount(), "Dialogue was ended in answer mode!"); //???allow?

				EventMgr->FireEvent(CStrID("OnDlgEnd"));

				UNSUBSCRIBE_EVENT(OnDlgAnswersBegin);

				if (ForegroundDlg.PlrSpeaker.IsValid())
					EntityMgr->GetEntity(ForegroundDlg.PlrSpeaker)->FireEvent(CStrID("EnableInput"));

				//FocusMgr->SetCameraFocusEntity(FocusMgr->GetInputFocusEntity());

				ForegroundDlg.Dlg = NULL;
				break;
			}
		}
	}

	for (nArray<CActiveDlg>::iterator pDlg = BackgroundDlgs.Begin(); pDlg != BackgroundDlgs.End(); )
	{
		CDlgNode* pNewNode = pDlg->Trigger();
		while (pNewNode != pDlg->pCurrNode)
		{
			if (pNewNode)
			{
				pDlg->EnterNode(pNewNode);
				pNewNode = pDlg->Trigger();
			}
			else
			{
				BackgroundDlgs.Erase(pDlg);
				pDlg--;
				break;
			}
		}
		pDlg++;
	}
}
//---------------------------------------------------------------------

void CDialogueManager::SayPhrase(CStrID SpeakerEntity, const nString& Phrase, CActiveDlg& Dlg)
{
	if (SpeakerEntity == "$DlgOwner") SpeakerEntity = Dlg.DlgOwner;
	else if (SpeakerEntity == "$PlrSpeaker") SpeakerEntity = Dlg.PlrSpeaker;
	PEntity Speaker = EntityMgr->GetEntity(SpeakerEntity, true);
	if (!Speaker.IsValid())
		n_error("CDialogueManager::SayPhrase -> speaker entity '%s' not found", SpeakerEntity.CStr());

	if (IsDialogueForeground(Dlg))
	{
		nString SpeakerName;
		if (!Speaker->GetAttr<nString>(SpeakerName, CStrID("Name")))
			SpeakerName = Speaker->GetUID().CStr();
		//FocusMgr->SetCameraFocusEntity(Speaker);

		PParams P = n_new(CParams);
		P->Set(CStrID("SpeakerName"), (PVOID)SpeakerName.CStr());
		P->Set(CStrID("Phrase"), (PVOID)Phrase.CStr());
		EventMgr->FireEvent(CStrID("OnDlgPhrase"), P);
	}
	else
	{
		//!!!get timeout from the node!
		Speaker->GetProperty<Prop::CPropTalking>()->SayPhrase(CStrID(Phrase.CStr()));
	}
}
//---------------------------------------------------------------------

bool CDialogueManager::OnDlgAnswersBegin(const Events::CEventBase& Event)
{
	n_assert2(!ForegroundDlg.ValidLinkIndices.GetCount(), "Re-entering answer mode in dialogue");
	PEntity Speaker = EntityMgr->GetEntity(ForegroundDlg.PlrSpeaker);
	n_assert(Speaker.IsValid());
	//FocusMgr->SetCameraFocusEntity(Speaker);
	OK;
}
//---------------------------------------------------------------------

void CDialogueManager::ContinueDialogue(int ValidLinkIdx)
{
	ForegroundDlg.Continued = true;
	if (ValidLinkIdx >= 0 && ValidLinkIdx < ForegroundDlg.ValidLinkIndices.GetCount())
		ForegroundDlg.LinkIdx = ForegroundDlg.ValidLinkIndices[ValidLinkIdx];
	//EventMgr->FireEvent(CStrID("OnDlgAnswerSelected"));
}
//---------------------------------------------------------------------

} //namespace Story