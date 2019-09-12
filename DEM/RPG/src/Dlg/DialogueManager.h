#pragma once
#ifndef __DEM_L3_DLG_SYSTEM_H__
#define __DEM_L3_DLG_SYSTEM_H__

#include <Data/Singleton.h>
#include <Dlg/DlgContext.h>
#include <Data/Dictionary.h>

// Dialogue manager loads, stores and executes dialogues

namespace Data
{
	class CParams;
}

namespace Game
{
	class CEntity;
}

namespace Story
{

enum EDlgMode
{
	DlgMode_Foreground,	// Main UI window, single instance
	DlgMode_Background,	// Phrases above characters' heads, multiple instances
	DlgMode_Auto		// Decide from participators' belonging to Party or NPC
};

#define DlgMgr Story::CDialogueManager::Instance()

class CDialogueManager: public Core::CObject
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CDialogueManager);

private:

	CDict<CStrID, PDlgGraph>	DlgRegistry;
	CDict<CStrID, CDlgContext>	RunningDlgs;	// Indexed by initiator
	CStrID						ForegroundDlgID;

public:

	CDialogueManager() { __ConstructSingleton; }
	~CDialogueManager() { __DestructSingleton; }

	void			Trigger();

	PDlgGraph		CreateDialogueGraph(const Data::CParams& Params);
	PDlgGraph		GetDialogueGraph(CStrID ID);

	bool			RequestDialogue(CStrID Initiator, CStrID Target, EDlgMode Mode = DlgMode_Auto);
	bool			AcceptDialogue(CStrID ID, CStrID Target);
	bool			RejectDialogue(CStrID ID, CStrID Target);
	void			CloseDialogue(CStrID ID);

	CDlgContext*	GetDialogue(CStrID ID);
	EDlgState		GetDialogueState(CStrID ID);
	bool			IsForegroundDialogueActive() const { return ForegroundDlgID.IsValid(); }
	bool			IsDialogueForeground(CStrID ID) const { return ID == ForegroundDlgID; }
};

inline CDlgContext* CDialogueManager::GetDialogue(CStrID ID)
{
	IPTR Idx = RunningDlgs.FindIndex(ID);
	return (Idx == INVALID_INDEX) ? nullptr : &RunningDlgs.ValueAt(Idx);
}
//---------------------------------------------------------------------

inline EDlgState CDialogueManager::GetDialogueState(CStrID ID)
{
	CDlgContext* pCtx = GetDialogue(ID);
	return pCtx ? pCtx->State : DlgState_None;
}
//---------------------------------------------------------------------

}

#endif
