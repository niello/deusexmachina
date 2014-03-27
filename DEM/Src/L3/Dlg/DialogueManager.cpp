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

bool CDialogueManager::RequestDialogue(CStrID Initiator, CStrID Target, EDlgMode Mode)
{
	n_assert(Initiator.IsValid() && Target.IsValid());

	if (RunningDlgs.Contains(Initiator))
	{
		n_printf("Error,Dlg: Entity '%s' has already started another dialogue\n", Initiator.CStr());
		FAIL;
	}

	//!!!???check if initiator or target is already talking?!

	RPG::CFaction* pFaction = FactionMgr->GetFaction(CStrID("Party"));
	bool InitiatorIsPlr = pFaction && pFaction->IsMember(Initiator);
	bool TargetIsPlr = pFaction && pFaction->IsMember(Target);

	if (Mode == DlgMode_Auto)
		Mode = (InitiatorIsPlr || TargetIsPlr) ? DlgMode_Foreground : DlgMode_Background;

	if (Mode == DlgMode_Foreground && IsForegroundDialogueActive())
	{
		n_printf("Error,Dlg: Trying to start new foreground dialogue when other is active\n");
		FAIL;
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

	if (!NewDlg.Dlg.IsValid()) FAIL;

	NewDlg.Initiator = Initiator;
	NewDlg.Target = Target;

	if (NewDlg.Dlg->ScriptFile.IsValid() && !NewDlg.Dlg->ScriptObj.IsValid())
	{
		NewDlg.Dlg->ScriptObj = n_new(Scripting::CScriptObject(Initiator.CStr(), "Dialogues"));
		NewDlg.Dlg->ScriptObj->Init(); // No special class
		if (NewDlg.Dlg->ScriptObj->LoadScriptFile(NewDlg.Dlg->ScriptFile) != Success)
			n_printf("Error loading script \"%s\" for dialogue", NewDlg.Dlg->ScriptFile.CStr());
	}

	if (NewDlg.Dlg->ScriptObj.IsValid()) NewDlg.Dlg->ScriptObj->RunFunction("OnStart"); //???send dlg id and actor ids?!

	if (Mode == DlgMode_Foreground)
	{
		ForegroundDlgID = Initiator;

		//???!!!redesign!?
		if (NewDlg.PlrSpeaker.IsValid())
			EntityMgr->GetEntity(NewDlg.PlrSpeaker)->FireEvent(CStrID("DisableInput"));

		EventSrv->FireEvent(CStrID("OnDlgStart"));
	}

	RunningDlgs.Add(Initiator, NewDlg);
	OK;
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
	//!!!count time for dialogues where node waits for a timeout end!
	Core::Error(__FUNCTION__ " IMPLEMENT ME!!!");
}
//---------------------------------------------------------------------

void CDialogueManager::HandleNode(CDlgContext& Context)
{
	if (!Context.pCurrNode || !Context.pCurrNode->Phrase.IsValid()) return;

	CStrID SpeakerEntity = Context.pCurrNode->SpeakerEntity;

	if (SpeakerEntity == "$DlgOwner") SpeakerEntity = Context.DlgOwner;
	else if (SpeakerEntity == "$PlrSpeaker") SpeakerEntity = Context.PlrSpeaker;
	Game::PEntity Speaker = EntityMgr->GetEntity(SpeakerEntity, true);
	if (!Speaker.IsValid())
		Core::Error("CDialogueManager::SayPhrase -> speaker entity '%s' not found", SpeakerEntity.CStr());

	if (IsDialogueForeground(Context.Initiator))
	{
		CString SpeakerName;
		if (!Speaker->GetAttr<CString>(SpeakerName, CStrID("Name")))
			SpeakerName = Speaker->GetUID().CStr();
		//FocusMgr->SetCameraFocusEntity(Speaker);

		Data::PParams P = n_new(Data::CParams);
		P->Set(CStrID("SpeakerName"), (PVOID)SpeakerName.CStr());
		P->Set(CStrID("Phrase"), (PVOID)Context.pCurrNode->Phrase.CStr());
		EventSrv->FireEvent(CStrID("OnDlgPhrase"), P);
	}
	else
	{
		//!!!get timeout from the node!
		Speaker->GetProperty<Prop::CPropTalking>()->SayPhrase(CStrID(Context.pCurrNode->Phrase.CStr()));
	}
}
//---------------------------------------------------------------------

}