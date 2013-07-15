#pragma once
#ifndef __DEM_L3_DLG_SYSTEM_H__
#define __DEM_L3_DLG_SYSTEM_H__

#include <Core/RefCounted.h>
#include <Core/Singleton.h>
#include <Data/Dictionary.h>
#include <Data/StringID.h>
#include <Events/EventsFwd.h>
#include "Dialogue.h"

// Dialogue system (parsing/executing machine).
// It can execute dialogue as foreground (using special UI and mode, interactive)
// or as background (non-interactive, using phrases).

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
using namespace Game;
using namespace Data;

class CActiveDlg
{
public:

	CStrID		DlgOwner;
	CStrID		PlrSpeaker;

	PDialogue	Dlg;

	CDlgNode*	pCurrNode;
	float		NodeEnterTime;
	int			LinkIdx;
	CArray<int>	ValidLinkIndices;		// For Random / Answers nodes
	bool		IsCheckingConditions;
	bool		Continued;

	CActiveDlg(): pCurrNode(NULL) {}

	void		EnterNode(CDlgNode* pNewNode);
	CDlgNode*	Trigger() { return pCurrNode->Trigger(*this); }
};

#define DlgMgr Story::CDialogueManager::Instance()

class CDialogueManager: public Core::CRefCounted
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CDialogueManager);

private:

	enum ENodeType
	{
		DLG_NODE_EMPTY = 0,
		DLG_NODE_PHRASE = 1,
		DLG_NODE_ANSWERS = 2,
		DLG_NODE_RANDOM = 3
	};

	CActiveDlg						ForegroundDlg;
	CArray<CActiveDlg>				BackgroundDlgs;

	CDict<CStrID, PDialogue>	DlgRegistry;

	DECLARE_EVENT_HANDLER(OnDlgAnswersBegin, OnDlgAnswersBegin);

public:

	CDialogueManager() { __ConstructSingleton; }
	~CDialogueManager() { __DestructSingleton; }

	void		Trigger();

	PDialogue	CreateDialogue(const CParams& Params, const CString& Name);
	PDialogue	GetDialogue(const CString& Name); //???CStrID identifier?

	void		StartDialogue(CEntity* pTarget, CEntity* pInitiator, bool Foreground);
	void		ContinueDialogue(int ValidLinkIdx = -1);
	bool		IsDialogueActive() const { return ForegroundDlg.Dlg.IsValid(); }
	bool		IsDialogueForeground(CActiveDlg& Dlg) const { return &Dlg == &ForegroundDlg; }
	//AbortDlg(ID);

	void		SayPhrase(CStrID SpeakerEntity, const CString& Phrase, CActiveDlg& Dlg);
};

}

#endif
