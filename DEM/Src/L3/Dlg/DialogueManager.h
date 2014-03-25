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
	Dlg_Foreground,	// Main UI window, single instance
	Dlg_Background,	// Phrases above characters' heads, multiple instances
	Dlg_Auto		// Decide from participators' belonging to Party or NPC
};

#define DlgMgr Story::CDialogueManager::Instance()

class CDialogueManager: public Core::CRefCounted
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CDialogueManager);

private:

	CDict<CStrID, PDlgGraph>	DlgRegistry;
	CDict<CStrID, CDlgContext>	RunningDlgs;
	CStrID						ForegroundDlg;

public:

	CDialogueManager() { __ConstructSingleton; }
	~CDialogueManager() { __DestructSingleton; }

	void		Trigger();

	PDlgGraph	CreateDialogueGraph(const Data::CParams& Params);
	PDlgGraph	GetDialogueGraph(CStrID ID);

	CStrID		RequestDialogue(CStrID Initiator, CStrID Target, EDlgMode Mode);
	void		AcceptDialogue(CStrID Target, CStrID DlgID);
	void		RejectDialogue(CStrID Target, CStrID DlgID);
	void		CloseDialogue(CStrID DlgID);
	EDlgState	GetDialogueState(CStrID DlgID) const;

	void		HandleNode(CDlgContext& Context);
	void		ContinueDialogue(int ValidLinkIdx = -1);
	//???!!!FollowLink(CStrID ID, int Idx);?!

	bool		IsDialogueActive(CStrID ID) const { return RunningDlgs.Contains(ID); }
	bool		IsForegroundDialogueActive() const { return ForegroundDlg.IsValid(); }
	bool		IsDialogueForeground(CStrID ID) const { return ID == ForegroundDlg; }
};

}

#endif
