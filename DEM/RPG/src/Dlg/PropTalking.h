#pragma once
#include <Game/Property.h>
#include <StdDEM.h>

// Adds ability to talk to the actor. Actor can speak in a conversation or spell
// background phrases, that appear above its head/top.

namespace Story
{
	typedef Ptr<class CDlgGraph> PDlgGraph;
}

namespace Prop
{

class CPropTalking: public Game::CProperty
{
	FACTORY_CLASS_DECL;
	__DeclarePropertyStorage;

protected:

	//??????!!!!!or just CStrID dlg name & resolve in DlgMgr
	Story::PDlgGraph Dialogue;

	//broken dialogue state (to restart or take into account)

	virtual bool	InternalActivate();
	virtual void	InternalDeactivate();
	void			EnableSI(class CPropScriptable& Prop);
	void			DisableSI(class CPropScriptable& Prop);

	DECLARE_EVENT_HANDLER(OnPropActivated, OnPropActivated);
	DECLARE_EVENT_HANDLER(OnPropDeactivating, OnPropDeactivating);
	DECLARE_EVENT_HANDLER(OnSOActionStart, OnSOActionStart);
	//!!!OnDlgNodeEnter - SayPhrase!

public:

	virtual void		SayPhrase(CStrID PhraseID);

	Story::CDlgGraph*	GetDialogue() { return Dialogue.Get(); }
};

}
