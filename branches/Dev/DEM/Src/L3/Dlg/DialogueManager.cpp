#include "DialogueManager.h"

#include <Dlg/DlgNode.h>
#include <Dlg/PropTalking.h>
#include <Factions/FactionManager.h>
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <Data/DataServer.h>
#include <Scripting/ScriptServer.h>
#include <Events/EventServer.h>
#include <Game/EntityManager.h>
#include <Game/GameServer.h>

namespace Story
{
__ImplementClassNoFactory(Story::CDialogueManager, Core::CRefCounted);
__ImplementSingleton(Story::CDialogueManager);

//???use DSS for PRM?
PDlgGraph CDialogueManager::CreateDialogueGraph(const Data::CParams& Params)
{
	CDict<CStrID, PDlgNode> LoadedNodes;

	PDlgGraph Dlg = n_new(CDlgGraph);

	const Data::PParams& Nodes = Params.Get<Data::PParams>(CStrID("Nodes"));
	for (int i = 0; i < Nodes->GetCount(); ++i)
	{
		const Data::CParam& Prm = Nodes->Get(i);
		const Data::CParams& NodeDesc = *Prm.GetValue<Data::PParams>();
		int Type = NodeDesc.Get<int>(CStrID("Type"));
		PDlgNode NewNode = n_new(CDlgNode);
		NewNode->SpeakerEntity = NodeDesc.Get<CStrID>(CStrID("Speaker"), CStrID::Empty);
		NewNode->Phrase = NodeDesc.Get<CString>(CStrID("Phrase"), CString::Empty);
		LoadedNodes.Add(Prm.GetName(), NewNode);
		Dlg->Nodes.Add(NewNode);
	}
	
	Dlg->StartNode = LoadedNodes[Params.Get<CStrID>(CStrID("StartNode"))];

	bool UsesScript = false;

	for (int i = 0; i < Nodes->GetCount(); ++i)
	{
		const Data::CParam& Prm = Nodes->Get(i);
		const Data::CParams& NodeDesc = *Prm.GetValue<Data::PParams>();

		Data::PDataArray Links;
		if (!NodeDesc.Get<Data::PDataArray>(Links, CStrID("Links")) || !Links->GetCount()) continue;

		CDlgNode* pFrom = LoadedNodes[Prm.GetName()].GetUnsafe();

		CDlgNode::CLink* pLink = pFrom->Links.Reserve(Links->GetCount());
		for (int j = 0; j < Links->GetCount(); ++j, ++pLink)
		{
			const Data::CParams& LinkDesc = *Links->Get<Data::PParams>(j);

			int ToIdx = LinkDesc.IndexOf(CStrID("To"));
			pLink->pTargetNode = (ToIdx == INVALID_INDEX) ? NULL : LoadedNodes[LinkDesc.Get<CStrID>(ToIdx)];

			pLink->Condition = LinkDesc.Get<CString>(CStrID("Condition"), NULL);
			pLink->Action = LinkDesc.Get<CString>(CStrID("Action"), NULL);

			if (!UsesScript && (pLink->Condition.IsValid() || pLink->Action.IsValid())) UsesScript = true;
		}
	}

	if (UsesScript)
	{
		const CString& Script = Params.Get<CString>(CStrID("Script"), NULL);
		if (Script.IsValid()) Dlg->ScriptFile = CString("Scripts:") + Script + ".lua";
	}

	return Dlg;
}
//---------------------------------------------------------------------

PDlgGraph CDialogueManager::GetDialogueGraph(CStrID ID)
{
	int Idx = DlgRegistry.FindIndex(ID);
	if (Idx > -1) return DlgRegistry.ValueAt(Idx);
	else
	{
		Data::PParams Desc = DataSrv->LoadPRM(CString("Dlg:") + ID.CStr() + ".prm", false);
		return Desc.IsValid() ? DlgRegistry.Add(ID, CreateDialogueGraph(*Desc)) : NULL;
	}
}
//---------------------------------------------------------------------

//???how to assign ID? request from the outside or set here and return?
CStrID CDialogueManager::RequestDialogue(CStrID Initiator, CStrID Target, EDlgMode Mode)
{
	n_assert(Initiator.IsValid() && Target.IsValid());

	//!!!???check if initiator or target is already talking?!

	RPG::CFaction* pFaction = FactionMgr->GetFaction(CStrID("Party"));
	bool InitiatorIsPlr = pFaction && pFaction->IsMember(Initiator);
	bool TargetIsPlr = pFaction && pFaction->IsMember(Target);

	if (Mode == Dlg_Auto)
		Mode = (InitiatorIsPlr || TargetIsPlr) ? Dlg_Foreground : Dlg_Background;

	if (Mode == Dlg_Foreground && IsForegroundDialogueActive())
	{
		n_printf("Error,Dlg: Trying to start new foreground dialogue when other is active.");
		return CStrID::Empty;
	}

	CDlgContext NewDlg;

	if (TargetIsPlr == InitiatorIsPlr) // Plr-Plr & NPC-NPC
	{
		NewDlg.Dlg = EntityMgr->GetEntity(Target)->GetProperty<Prop::CPropTalking>()->GetDialogue();
		if (NewDlg.Dlg.IsValid())
		{
			NewDlg.DlgOwner = Target;
			if (InitiatorIsPlr) NewDlg.PlrSpeaker = Initiator;
		}
		else
		{
			NewDlg.Dlg = EntityMgr->GetEntity(Initiator)->GetProperty<Prop::CPropTalking>()->GetDialogue();
			if (NewDlg.Dlg.IsValid())
			{
				NewDlg.DlgOwner = Initiator;
				if (TargetIsPlr) NewDlg.PlrSpeaker = Target;
			}
		}
	}
	else // NPC-Plr & Plr-NPC
	{
		CStrID NPC = TargetIsPlr ? Initiator : Target;
		CStrID Plr = TargetIsPlr ? Target : Initiator;
		NewDlg.Dlg = EntityMgr->GetEntity(NPC)->GetProperty<Prop::CPropTalking>()->GetDialogue();
		if (NewDlg.Dlg.IsValid())
		{
			NewDlg.DlgOwner = NPC;
			NewDlg.PlrSpeaker = Plr;
		}
		else //???need? NPC talking with Player with player-attached dlg
		{
			NewDlg.Dlg = EntityMgr->GetEntity(Plr)->GetProperty<Prop::CPropTalking>()->GetDialogue();
			if (NewDlg.Dlg.IsValid())
			{
				NewDlg.DlgOwner = Plr;
				NewDlg.PlrSpeaker = NPC;
			}
		}
	}

	if (!NewDlg.Dlg.IsValid()) return CStrID::Empty;

	CString IDStr(Initiator.CStr());
	IDStr.Add(Target.CStr());
	NewDlg.ID = CStrID(IDStr.CStr());

	n_assert(!RunningDlgs.Contains(NewDlg.ID));

	if (NewDlg.Dlg->ScriptFile.IsValid() && !NewDlg.Dlg->ScriptObj.IsValid())
	{
		NewDlg.Dlg->ScriptObj = n_new(Scripting::CScriptObject(NewDlg.ID.CStr(), "Dialogues"));
		NewDlg.Dlg->ScriptObj->Init(); // No special class
		if (NewDlg.Dlg->ScriptObj->LoadScriptFile(NewDlg.Dlg->ScriptFile) != Success)
			n_printf("Error loading script \"%s\" for dialogue", NewDlg.Dlg->ScriptFile.CStr());
	}

	if (NewDlg.Dlg->ScriptObj.IsValid()) NewDlg.Dlg->ScriptObj->RunFunction("OnStart"); //???send dlg id and actor ids?!

	if (Mode == Dlg_Foreground)
	{
		ForegroundDlg = NewDlg.ID;

		//???!!!redesign!?
		if (NewDlg.PlrSpeaker.IsValid())
			EntityMgr->GetEntity(NewDlg.PlrSpeaker)->FireEvent(CStrID("DisableInput"));

		EventSrv->FireEvent(CStrID("OnDlgStart"));
	}

	RunningDlgs.Add(NewDlg.ID, NewDlg);
}
//---------------------------------------------------------------------

void CDialogueManager::AcceptDialogue(CStrID Target, CStrID DlgID)
{
	//get dlg
	//CDlgContext& Ctx = 
	//find waiting slot (for now can just store state Waiting or Accepted)
	//remove waiting creature, mb add to list of participants
	//if all required characters accepted dlg, start from start node
	//Ctx.Dlg->StartNode->OnEnter(Ctx);
}
//---------------------------------------------------------------------

void CDialogueManager::RejectDialogue(CStrID Target, CStrID DlgID)
{
	//get dlg
	//CDlgContext& Ctx = 
	//find waiting slot
	//if actor is listed, remove this context from running dlgs, notify end and set result to Failure
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
				EventSrv->FireEvent(CStrID("OnDlgNoCmdAvailable"));
				pNewNode = ForegroundDlg.Trigger();
			}
			else
			{
				n_assert2(!ForegroundDlg.ValidLinkIndices.GetCount(), "Dialogue was ended in answer mode!"); //???allow?

				EventSrv->FireEvent(CStrID("OnDlgEnd"));

				if (ForegroundDlg.PlrSpeaker.IsValid())
					EntityMgr->GetEntity(ForegroundDlg.PlrSpeaker)->FireEvent(CStrID("EnableInput"));

				//FocusMgr->SetCameraFocusEntity(FocusMgr->GetInputFocusEntity());

				ForegroundDlg.Dlg = NULL;
				break;
			}
		}
	}

	for (CArray<CDlgContext>::CIterator pDlg = BackgroundDlgs.Begin(); pDlg != BackgroundDlgs.End(); )
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
				BackgroundDlgs.Remove(pDlg);
				pDlg--;
				break;
			}
		}
		pDlg++;
	}
}
//---------------------------------------------------------------------

void CDialogueManager::HandleNode(CDlgContext& Context)
{
	if (!Context.pCurrNode || !Context.pCurrNode->Phrase.IsValid()) return;

	CStrID SpeakerEntity = Context.pCurrNode->SpeakerEntity;

	if (SpeakerEntity == "$DlgOwner") SpeakerEntity = Dlg.DlgOwner;
	else if (SpeakerEntity == "$PlrSpeaker") SpeakerEntity = Dlg.PlrSpeaker;
	PEntity Speaker = EntityMgr->GetEntity(SpeakerEntity, true);
	if (!Speaker.IsValid())
		Core::Error("CDialogueManager::SayPhrase -> speaker entity '%s' not found", SpeakerEntity.CStr());

	if (IsDialogueForeground(Context.ID))
	{
		CString SpeakerName;
		if (!Speaker->GetAttr<CString>(SpeakerName, CStrID("Name")))
			SpeakerName = Speaker->GetUID().CStr();
		//FocusMgr->SetCameraFocusEntity(Speaker);

		Data::PParams P = n_new(Data::CParams);
		P->Set(CStrID("SpeakerName"), (PVOID)SpeakerName.CStr());
		P->Set(CStrID("Phrase"), (PVOID)Phrase.CStr());
		EventSrv->FireEvent(CStrID("OnDlgPhrase"), P);
	}
	else
	{
		//!!!get timeout from the node!
		Speaker->GetProperty<Prop::CPropTalking>()->SayPhrase(CStrID(Phrase.CStr()));
	}
}
//---------------------------------------------------------------------

void CDialogueManager::ContinueDialogue(int ValidLinkIdx)
{
	ForegroundDlg.Continued = true;
	if (ValidLinkIdx >= 0 && ValidLinkIdx < ForegroundDlg.ValidLinkIndices.GetCount())
		ForegroundDlg.LinkIdx = ForegroundDlg.ValidLinkIndices[ValidLinkIdx];
	//EventSrv->FireEvent(CStrID("OnDlgAnswerSelected"));
}
//---------------------------------------------------------------------

}