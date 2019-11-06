#include "DialogueManager.h"

#include <Dlg/PropTalking.h>
#include <Factions/FactionManager.h>
#include <Game/GameServer.h>
#include <Game/Entity.h>
#include <Data/DataArray.h>
#include <Data/ParamsUtils.h>
#include <Events/EventServer.h>

namespace Story
{
__ImplementClassNoFactory(Story::CDialogueManager, Core::CObject);
__ImplementSingleton(Story::CDialogueManager);

//???use DSS for PRM?
PDlgGraph CDialogueManager::CreateDialogueGraph(const Data::CParams& Params)
{
	CDict<CStrID, PDlgNode> LoadedNodes;

	PDlgGraph Dlg = n_new(CDlgGraph);

	const Data::PParams& Nodes = Params.Get<Data::PParams>(CStrID("Nodes"));
	for (UPTR i = 0; i < Nodes->GetCount(); ++i)
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

	const CString& Script = Params.Get<CString>(CStrID("Script"), CString::Empty);
	if (Script.IsValid()) Dlg->ScriptFile = CString("Scripts:") + Script + ".lua";

	for (UPTR i = 0; i < Nodes->GetCount(); ++i)
	{
		const Data::CParam& Prm = Nodes->Get(i);
		const Data::CParams& NodeDesc = *Prm.GetValue<Data::PParams>();

		Data::PDataArray Links;
		if (!NodeDesc.Get<Data::PDataArray>(Links, CStrID("Links")) || !Links->GetCount()) continue;

		CDlgNode* pFrom = LoadedNodes[Prm.GetName()].Get();

		CDlgNode::CLink* pLink = pFrom->Links.Reserve(Links->GetCount());
		for (UPTR j = 0; j < Links->GetCount(); ++j, ++pLink)
		{
			const Data::CParams& LinkDesc = *Links->Get<Data::PParams>(j);

			int ToIdx = LinkDesc.IndexOf(CStrID("To"));
			pLink->pTargetNode = (ToIdx == INVALID_INDEX) ? nullptr : LoadedNodes[LinkDesc.Get<CStrID>(ToIdx)];

			pLink->Condition = LinkDesc.Get<CString>(CStrID("Condition"), CString::Empty);
			pLink->Action = LinkDesc.Get<CString>(CStrID("Action"), CString::Empty);
		}
	}

	return Dlg;
}
//---------------------------------------------------------------------

PDlgGraph CDialogueManager::GetDialogueGraph(CStrID ID)
{
	IPTR Idx = DlgRegistry.FindIndex(ID);
	if (Idx > -1) return DlgRegistry.ValueAt(Idx);
	else
	{
		Data::PParams Desc = ParamsUtils::LoadParamsFromPRM(CString("Dlg:") + ID.CStr() + ".prm");
		return Desc ? DlgRegistry.Add(ID, CreateDialogueGraph(*Desc)) : nullptr;
	}
}
//---------------------------------------------------------------------

bool CDialogueManager::RequestDialogue(CStrID Initiator, CStrID Target, EDlgMode Mode)
{
	n_assert(Initiator.IsValid() && Target.IsValid());

	if (RunningDlgs.Contains(Initiator))
	{
		Sys::Log("Error,Dlg: Entity '%s' has already started another dialogue\n", Initiator.CStr());
		FAIL;
	}

	n_assert_dbg(GameSrv->GetEntityMgr()->EntityExists(Initiator));

	Game::CEntity* pTargetEnt = GameSrv->GetEntityMgr()->GetEntity(Target);
	if (!pTargetEnt)
	{
		Sys::Log("Error,Dlg: Entity '%s' doesn't exist\n", Target.CStr());
		FAIL;
	}

	RPG::CFaction* pFaction = FactionMgr->GetFaction(CStrID("Party"));
	bool InitiatorIsPlr = pFaction && pFaction->IsMember(Initiator);
	bool TargetIsPlr = pFaction && pFaction->IsMember(Target);

	if (Mode == DlgMode_Auto)
		Mode = (InitiatorIsPlr || TargetIsPlr) ? DlgMode_Foreground : DlgMode_Background;

	if (Mode == DlgMode_Foreground && IsForegroundDialogueActive())
	{
		Sys::Log("Error,Dlg: Trying to start new foreground dialogue when other is active\n");
		FAIL;
	}

	CDlgContext NewDlg;

	if (TargetIsPlr == InitiatorIsPlr) // Plr-Plr & NPC-NPC
	{
		NewDlg.Dlg = GameSrv->GetEntityMgr()->GetEntity(Target)->GetProperty<Prop::CPropTalking>()->GetDialogue();
		if (NewDlg.Dlg.IsValidPtr())
		{
			NewDlg.DlgOwner = Target;
			if (InitiatorIsPlr) NewDlg.PlrSpeaker = Initiator;
		}
		else
		{
			NewDlg.Dlg = GameSrv->GetEntityMgr()->GetEntity(Initiator)->GetProperty<Prop::CPropTalking>()->GetDialogue();
			if (NewDlg.Dlg.IsValidPtr())
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
		NewDlg.Dlg = GameSrv->GetEntityMgr()->GetEntity(NPC)->GetProperty<Prop::CPropTalking>()->GetDialogue();
		if (NewDlg.Dlg.IsValidPtr())
		{
			NewDlg.DlgOwner = NPC;
			NewDlg.PlrSpeaker = Plr;
		}
		else //???need? NPC talking with Player with player-attached dlg
		{
			NewDlg.Dlg = GameSrv->GetEntityMgr()->GetEntity(Plr)->GetProperty<Prop::CPropTalking>()->GetDialogue();
			if (NewDlg.Dlg.IsValidPtr())
			{
				NewDlg.DlgOwner = Plr;
				NewDlg.PlrSpeaker = NPC;
			}
		}
	}

	if (NewDlg.Dlg.IsNullPtr()) FAIL;

	NewDlg.Initiator = Initiator;
	NewDlg.Target = Target;
	NewDlg.State = DlgState_Requested;
	RunningDlgs.Add(Initiator, NewDlg);

	if (Mode == DlgMode_Foreground) ForegroundDlgID = Initiator;

	if (NewDlg.Dlg->ScriptFile.IsValid() && !NewDlg.Dlg->ScriptObj.IsValidPtr())
	{
		NewDlg.Dlg->ScriptObj = n_new(Scripting::CScriptObject(Initiator.CStr(), "Dialogues"));
		NewDlg.Dlg->ScriptObj->Init(); // No special class
		if (NewDlg.Dlg->ScriptObj->LoadScriptFile(NewDlg.Dlg->ScriptFile) != Success)
			Sys::Log("Error,Dlg: Error loading script \"%s\" for dialogue", NewDlg.Dlg->ScriptFile.CStr());
	}

	Data::PParams P = n_new(Data::CParams(1));
	P->Set(CStrID("Initiator"), Initiator);
	pTargetEnt->FireEvent(CStrID("OnDlgRequest"), P);

	OK;
}
//---------------------------------------------------------------------

bool CDialogueManager::AcceptDialogue(CStrID ID, CStrID Target)
{
	IPTR Idx = RunningDlgs.FindIndex(ID);
	if (Idx == INVALID_INDEX) FAIL;
	CDlgContext& Ctx = RunningDlgs.ValueAt(Idx);
	if (Ctx.Target != Target) FAIL;
	Ctx.pCurrNode = Ctx.Dlg->StartNode;
	Ctx.State = DlgState_InNode;

	//???send dlg id and actor ids?!
	if (Ctx.Dlg->ScriptObj.IsValidPtr()) Ctx.Dlg->ScriptObj->RunFunction("OnStart");

	Data::PParams P = n_new(Data::CParams(3));
	P->Set(CStrID("Initiator"), Ctx.Initiator);
	P->Set(CStrID("Target"), Target);
	P->Set(CStrID("IsForeground"), IsDialogueForeground(ID));
	EventSrv->FireEvent(CStrID("OnDlgStart"), P);

	OK;
}
//---------------------------------------------------------------------

bool CDialogueManager::RejectDialogue(CStrID ID, CStrID Target)
{
	IPTR Idx = RunningDlgs.FindIndex(ID);
	if (Idx == INVALID_INDEX) FAIL;
	CDlgContext& Ctx = RunningDlgs.ValueAt(Idx);
	if (Ctx.Target != Target) FAIL;
	Ctx.State = DlgState_Aborted;

	//Data::PParams P = n_new(Data::CParams(3));
	//P->Set(CStrID("Initiator"), Ctx.Initiator);
	//P->Set(CStrID("Target"), Target);
	//P->Set(CStrID("IsForeground"), IsDialogueForeground(ID));
	//EventSrv->FireEvent(CStrID("OnDlgReject"), P);

	OK;
}
//---------------------------------------------------------------------

void CDialogueManager::CloseDialogue(CStrID ID)
{
	//???kill script?
	RunningDlgs.Remove(ID);
}
//---------------------------------------------------------------------

void CDialogueManager::Trigger()
{
	for (UPTR i = 0; i < RunningDlgs.GetCount(); ++i)
	{
		CDlgContext& Ctx = RunningDlgs.ValueAt(i);
		bool IsForeground = IsDialogueForeground(Ctx.Initiator);
		Ctx.Trigger(IsForeground);
		if (IsForeground && (Ctx.State == DlgState_Finished || Ctx.State == DlgState_Aborted))
			ForegroundDlgID = CStrID::Empty;
	}
}
//---------------------------------------------------------------------

}