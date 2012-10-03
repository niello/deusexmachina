#pragma once
#ifndef __DEM_L3_DLG_SYSTEM_H__
#define __DEM_L3_DLG_SYSTEM_H__

#include <Core/RefCounted.h>
#include <util/ndictionary.h>
#include <Data/StringID.h>
#include <Events/Events.h>
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
	nArray<int>	ValidLinkIndices;		// For Random / Answers nodes
	bool		IsCheckingConditions;
	bool		Continued;

	CActiveDlg(): pCurrNode(NULL) {}

	void		EnterNode(CDlgNode* pNewNode);
	CDlgNode*	Trigger() { return pCurrNode->Trigger(*this); }
};

#define DlgSys Story::CDlgSystem::Instance()

class CDlgSystem: public Core::CRefCounted
{
	DeclareRTTI;
	DeclareFactory(CDlgSystem);
	__DeclareSingleton(CDlgSystem);

private:

	enum ENodeType
	{
		DLG_NODE_EMPTY = 0,
		DLG_NODE_PHRASE = 1,
		DLG_NODE_ANSWERS = 2,
		DLG_NODE_RANDOM = 3
	};

	CActiveDlg						ForegroundDlg;
	nArray<CActiveDlg>				BackgroundDlgs;

	nDictionary<CStrID, PDialogue>	DlgRegistry;

	DECLARE_EVENT_HANDLER(OnDlgAnswersBegin, OnDlgAnswersBegin);

public:

	CDlgSystem();
	~CDlgSystem();

	void		Trigger();

	PDialogue	CreateDialogue(const CParams& Params, const nString& Name);
	PDialogue	GetDialogue(const nString& Name); //???CStrID identifier?

	void		StartDialogue(CEntity* pTarget, CEntity* pInitiator, bool Foreground);
	void		ContinueDialogue(int ValidLinkIdx = -1);
	bool		IsDialogueActive() const { return ForegroundDlg.Dlg.isvalid(); }
	bool		IsDialogueForeground(CActiveDlg& Dlg) const { return &Dlg == &ForegroundDlg; }
	//AbortDlg(ID);

	void		SayPhrase(CStrID SpeakerEntity, const nString& Phrase, CActiveDlg& Dlg);
};

RegisterFactory(CDlgSystem);

}

#endif
