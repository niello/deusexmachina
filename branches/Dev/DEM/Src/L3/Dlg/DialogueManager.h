#pragma once
#ifndef __DEM_L3_DLG_SYSTEM_H__
#define __DEM_L3_DLG_SYSTEM_H__

#include <Core/Singleton.h>
#include <Data/Dictionary.h>
#include <Data/StringID.h>
#include <Events/EventsFwd.h>
#include <Dlg/DlgContext.h>

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

enum EDlgState
{
	DlgState_Requested,
	DlgState_Running,
	DlgState_Finished,
	DlgState_Aborted
};

#define DlgMgr Story::CDialogueManager::Instance()

class CDialogueManager: public Core::CRefCounted
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

	void		Trigger();

	PDlgGraph	CreateDialogueGraph(const Data::CParams& Params);
	PDlgGraph	GetDialogueGraph(CStrID ID);

	bool		RequestDialogue(CStrID Initiator, CStrID Target, EDlgMode Mode = DlgMode_Auto);
	void		AcceptDialogue(CStrID Target, CStrID DlgID);
	void		RejectDialogue(CStrID Target, CStrID DlgID);
	void		CloseDialogue(CStrID DlgID);
	EDlgState	GetDialogueState(CStrID DlgID) const;

	void		HandleNode(CDlgContext& Context);
	//!!!FollowLink(ID, Idx)!

	bool		IsDialogueActive(CStrID ID) const { return RunningDlgs.Contains(ID); }
	bool		IsForegroundDialogueActive() const { return ForegroundDlgID.IsValid(); }
	bool		IsDialogueForeground(CStrID ID) const { return ID == ForegroundDlgID; }
};

}

#endif
