#pragma once
#ifndef __DEM_L3_PROP_TALKING_H__
#define __DEM_L3_PROP_TALKING_H__

#include <Game/Property.h>
#include <DB/AttrID.h>
#include <StdDEM.h>

// Adds ability to talk to the actor. Actor can speak in a conversation or spell
// background phrases, that appear above its head/top.

namespace Story
{
	typedef Ptr<class CDialogue> PDialogue;
}

namespace Attr
{
	DeclareString(Dialogue);	// Default Dlg ID
}

namespace Properties
{
using namespace Events;
using namespace Story;

class CPropTalking: public Game::CProperty
{
	DeclareRTTI;
	DeclareFactory(CPropTalking);
	DeclarePropertyStorage;

protected:

	//??????!!!!!or just CStrID dlg name & resolve in DlgSys
	PDialogue Dialogue;

	//broken dialogue state (to restart or take into account)

	DECLARE_EVENT_HANDLER(ExposeSI, ExposeSI);
	DECLARE_EVENT_HANDLER(Talk, OnTalk);

public:

	virtual void	Activate();
	virtual void	Deactivate();
	virtual void	GetAttributes(nArray<DB::CAttrID>& Attrs);
	virtual void	SayPhrase(CStrID PhraseID);

	CDialogue*		GetDialogue() { return Dialogue.get_unsafe(); }
};

RegisterFactory(CPropTalking);

}

#endif