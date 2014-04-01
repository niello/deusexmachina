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
		PDlgNode NewNode = n_new(CDlgNode);
		NewNode->LinkMode = (CDlgNode::ELinkMode)NodeDesc.Get<int>(CStrID("Type"));
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

	n_assert_dbg(EntityMgr->EntityExists(Initiator));

	Game::CEntity* pTargetEnt = EntityMgr->GetEntity(Target);
	if (!pTargetEnt)
	{
		n_printf("Error,Dlg: Entity '%s' doesn't exist\n", Target.CStr());
		FAIL;
	}

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
	NewDlg.State = DlgState_Requested;
	RunningDlgs.Add(Initiator, NewDlg);

	if (Mode == DlgMode_Foreground) ForegroundDlgID = Initiator;

	if (NewDlg.Dlg->ScriptFile.IsValid() && !NewDlg.Dlg->ScriptObj.IsValid())
	{
		NewDlg.Dlg->ScriptObj = n_new(Scripting::CScriptObject(Initiator.CStr(), "Dialogues"));
		NewDlg.Dlg->ScriptObj->Init(); // No special class
		if (NewDlg.Dlg->ScriptObj->LoadScriptFile(NewDlg.Dlg->ScriptFile) != Success)
			n_printf("Error,Dlg: Error loading script \"%s\" for dialogue", NewDlg.Dlg->ScriptFile.CStr());
	}

	Data::PParams P = n_new(Data::CParams(1));
	P->Set(CStrID("Initiator"), Initiator);
	pTargetEnt->FireEvent(CStrID("OnDlgRequested"), P);

	OK;
}
//---------------------------------------------------------------------

bool CDialogueManager::AcceptDialogue(CStrID ID, CStrID Target)
{
	int Idx = RunningDlgs.FindIndex(ID);
	if (Idx == INVALID_INDEX) FAIL;
	CDlgContext& Ctx = RunningDlgs.ValueAt(Idx);
	if (Ctx.Target != Target) FAIL;
	Ctx.pCurrNode = Ctx.Dlg->StartNode;
	Ctx.State = DlgState_InNode;

	//???catch event instead?
	//request and start are different now!
	if (Ctx.Dlg->ScriptObj.IsValid()) Ctx.Dlg->ScriptObj->RunFunction("OnStart"); //???send dlg id and actor ids?!

	if (IsDialogueForeground(ID))
	{
		//???!!!redesign!?
		//???catch dlg start event in Plr script class and disable input there?
		//so no need to check plr speaker
		if (Ctx.PlrSpeaker.IsValid())
			EntityMgr->GetEntity(Ctx.PlrSpeaker)->FireEvent(CStrID("DisableInput"));

		//???event dlg start with bool prm foreground?
		EventSrv->FireEvent(CStrID("OnForegroundDlgStart"));
	}

	OK;
}
//---------------------------------------------------------------------

bool CDialogueManager::RejectDialogue(CStrID ID, CStrID Target)
{
	int Idx = RunningDlgs.FindIndex(ID);
	if (Idx == INVALID_INDEX) FAIL;
	CDlgContext& Ctx = RunningDlgs.ValueAt(Idx);
	if (Ctx.Target != Target) FAIL;
	Ctx.State = DlgState_Aborted;
	//???!!!notify dlg failed, or actor will read state
	OK;
}
//---------------------------------------------------------------------

void CDialogueManager::CloseDialogue(CStrID ID)
{
	Core::Error(__FUNCTION__ " IMPLEMENT ME!!!");
}
//---------------------------------------------------------------------

void CDialogueManager::Trigger()
{
	for (int i = 0; i < RunningDlgs.GetCount(); ++i)
	{
		CDlgContext& Ctx = RunningDlgs.ValueAt(i);

		//!!!if waiting, process wait!
		if (Ctx.State == DlgState_Waiting)// & waits timer)
		{
			//advance timer
			//if time has come,
				Ctx.State = DlgState_InLink;
		}

		//!!!can move it all into the CDlgContext, and call Ctx.Trigger(bool Foreground)!
		//or use virtual dlg handler and refuse diff between fore & back in Ctx
		while (Ctx.State == DlgState_InNode || Ctx.State == DlgState_InLink)
		{
			if (Ctx.State == DlgState_InNode)
			{
				Ctx.HandleNode();

				if (!Ctx.pCurrNode || !Ctx.pCurrNode->Phrase.IsValid()) return;

				CStrID SpeakerEntity = Ctx.pCurrNode->SpeakerEntity;
				if (SpeakerEntity == CStrID("$DlgOwner")) SpeakerEntity = Ctx.DlgOwner;
				else if (SpeakerEntity == CStrID("$PlrSpeaker")) SpeakerEntity = Ctx.PlrSpeaker;
				Game::PEntity Speaker = EntityMgr->GetEntity(SpeakerEntity, true);
				if (!Speaker.IsValid())
					Core::Error("CDialogueManager::SayPhrase -> speaker entity '%s' not found", SpeakerEntity.CStr());
 
				if (IsDialogueForeground(Ctx.Initiator))
				{
					CString SpeakerName;
					if (!Speaker->GetAttr<CString>(SpeakerName, CStrID("Name")))
						SpeakerName = Speaker->GetUID().CStr();
					//!!!can focus camera, or some special handler/script can catch global events and manage camera?

					//if UI stores Ctx or Ctx ID (Initiator ID) since init time, we even don't need to send any params
					EventSrv->FireEvent(CStrID("OnForegroundDlgNodeEnter"));

					//Data::PParams P = n_new(Data::CParams(2));
					//P->Set(CStrID("SpeakerName"), (PVOID)SpeakerName.CStr());
					//P->Set(CStrID("Phrase"), (PVOID)Ctx.pCurrNode->Phrase.CStr());
					//EventSrv->FireEvent(CStrID("OnDlgPhrase"), P);

					//!!!if no phrase and mode isn't answers, goto link immediately
				}
				else
				{
					n_assert2(Ctx.pCurrNode->LinkMode != CDlgNode::Link_Select, "Background dialogues don't support Link_Select!");

					//!!!get timeout from the node!
					//also can send entity event, catch it in CPropTalking
					Speaker->GetProperty<Prop::CPropTalking>()->SayPhrase(CStrID(Ctx.pCurrNode->Phrase.CStr()));

					if (Ctx.State == DlgState_Waiting)// & time to wait <= 0)
						Ctx.State = DlgState_InLink;
				}
			}

			if (Ctx.State == DlgState_InLink) Ctx.HandleLink();
		}
	}
}
//---------------------------------------------------------------------

}