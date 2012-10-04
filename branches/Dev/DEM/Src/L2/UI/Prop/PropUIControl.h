#pragma once
#ifndef __DEM_L2_PROP_UI_CONTROL_H__
#define __DEM_L2_PROP_UI_CONTROL_H__

#include <Game/Property.h>
#include <Events/Events.h>
#include <Events/EventHandler.h>
#include <DB/AttrID.h>
#include <util/ndictionary.h>

// InterActiveObject (AO, IAO) property. IAO is a 3D-world GUI entity similar to GUI controls like buttons.
// It has highlighting, hint and causes actions on activation, e.g. clicking.
// IAO uses physics/collide property to determine mouse-over etc.
// Later it can also have states, offering different action sets.

namespace Attr
{
	DeclareString(IAODesc);	// IAO description HRD file ID
	DeclareString(Name);	// UI (player-readable) name of IAO
};

namespace Data
{
	typedef Ptr<class CParams> PParams;
}

namespace Properties
{

class CPropUIControl: public Game::CProperty
{
	DeclareRTTI;
	DeclareFactory(CPropUIControl);
	DeclarePropertyStorage;

public:

	struct CAction
	{
		CStrID			ID;
		CStrID			EventID;	// cached
		nString			UIName;		//???use CSimpleString to reduce size of struct?
		short			Priority;
		bool			Enabled;	//!!!add methods to control it!
		bool			Visible;
		bool			AutoAdded;
		//???picture, or associate with ID? if so, UIName & Priority can also be associated with ID

		//!!!can also store handler HandleAction(CStrID ActionID)! (template functor?)
		Events::PSub	Sub;

		CAction(): Enabled(true), Visible(true), AutoAdded(false) {}
		CAction(CStrID _ID, LPCSTR Name, int _Priority): ID(_ID), UIName(Name), Priority(_Priority), Enabled(true), Visible(true) {}

		LPCSTR	GetUIName() const { return UIName.IsValid() ? UIName.Get() : ID.CStr(); }
		bool	operator <(const CAction& Other) const { return (Enabled && !Other.Enabled) || (Priority > Other.Priority); }
		bool	operator >(const CAction& Other) const { return (!Enabled && Other.Enabled) || (Priority < Other.Priority); }
	};

protected:

	nString			UIName;
	nArray<CAction>	Actions;
	Data::PParams	SOActionNames;
	bool			AutoAddSmartObjActions;

	bool		AddActionHandler(CStrID ID, LPCSTR UIName, Events::PEventHandler Handler, int Priority, bool AutoAdded = false);
	bool		ExecuteAction(Game::CEntity* pActorEnt, CAction& Action);
	bool		OnExecuteSmartObjAction(const Events::CEventBase& Event);
	CAction*	GetActionByID(CStrID ID);

	DECLARE_EVENT_HANDLER(OnPropsActivated, OnPropsActivated);
	DECLARE_EVENT_HANDLER(ExposeSI, ExposeSI);
	DECLARE_EVENT_HANDLER(ObjMouseOver, OnObjMouseOver);
	DECLARE_EVENT_HANDLER(OverrideUIName, OverrideUIName);
	DECLARE_EVENT_HANDLER(OnSOActionAvailabile, OnSOActionAvailabile);

public:

	enum
	{
		DEFAULT_PRIORITY = 20
	};

	virtual void			Activate();
	virtual void			Deactivate();
	virtual void			GetAttributes(nArray<DB::CAttrID>& Attrs);

	bool					AddActionHandler(CStrID ID, LPCSTR UIName, bool (*Callback)(const Events::CEventBase&), int Priority = DEFAULT_PRIORITY, bool AutoAdded = false);
	template<class T>
	bool					AddActionHandler(CStrID ID, LPCSTR UIName, T* Object, bool (T::*Callback)(const Events::CEventBase&), int Priority = DEFAULT_PRIORITY, bool AutoAdded = false);
	bool					AddActionHandler(CStrID ID, LPCSTR UIName, LPCSTR ScriptFuncName, int Priority = DEFAULT_PRIORITY, bool AutoAdded = false);
	void					RemoveActionHandler(CStrID ID);

	const nArray<CAction>&	GetActions() const { return Actions; }

	bool					ExecuteAction(Game::CEntity* pActorEnt, CStrID ID);
	bool					ExecuteDefaultAction(Game::CEntity* pActorEnt);
	void					ShowPopup(Game::CEntity* pActorEnt);
};

RegisterFactory(CPropUIControl);

inline bool CPropUIControl::AddActionHandler(CStrID ID, LPCSTR UIName,
											 bool (*Callback)(const Events::CEventBase&),
											 int Priority, bool AutoAdded)
{
	return AddActionHandler(ID, UIName, n_new(Events::CEventHandlerCallback)(Callback), Priority, AutoAdded);
}
//---------------------------------------------------------------------

template<class T>
inline bool CPropUIControl::AddActionHandler(CStrID ID, LPCSTR UIName, T* Object,
											 bool (T::*Callback)(const Events::CEventBase&),
											 int Priority, bool AutoAdded)
{
	return AddActionHandler(ID, UIName, n_new(Events::CEventHandlerMember<T>)(Object, Callback), Priority, AutoAdded);
}
//---------------------------------------------------------------------

inline CPropUIControl::CAction* CPropUIControl::GetActionByID(CStrID ID)
{
	for (nArray<CAction>::iterator It = Actions.Begin(); It != Actions.End(); It++)
		if (It->ID == ID) return It;
	return NULL;
}
//---------------------------------------------------------------------

}

#endif