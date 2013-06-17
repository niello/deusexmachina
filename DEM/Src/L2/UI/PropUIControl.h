#pragma once
#ifndef __DEM_L2_PROP_UI_CONTROL_H__
#define __DEM_L2_PROP_UI_CONTROL_H__

#include <Game/Property.h>
#include <Physics/NodeAttrCollision.h>
#include <Events/EventHandler.h>
#include <util/narray.h>

// InterActiveObject (AO, IAO) property. IAO is a 3D world GUI entity similar to GUI controls like buttons.
// It has highlighting, hint and causes actions on activation, e.g. clicking.
// This control can be detected by mouse through the physics raytest. For this to work, entity must have
// an attached physics shape which accepts collision with MousePick group. By default it accepts, to disable,
// use AllNoPick collision mask. Shape can be attached by CPropPhysics or right here, if object requires no physics.
// This control can automatically reflect actions available in a CPropSmartObject of the same entity.

namespace Data
{
	typedef Ptr<class CParams> PParams;
}

namespace Prop
{
class CPropSmartObject;
class CPropSceneNode;

class CPropUIControl: public Game::CProperty
{
	__DeclareClass(CPropUIControl);
	__DeclarePropertyStorage;

public:

	struct CAction
	{
		CStrID			ID;
		CStrID			EventID;	// cached
		nString			UIName;		//???use CSimpleString to reduce size of struct?
		short			Priority;	// The higher is value the closer an action to the top of the list
		bool			Enabled;	//!!!add methods to control it!
		bool			Visible;
		bool			IsSOAction;
		//???picture, or associate with ID? if so, UIName & Priority can also be associated with ID

		//!!!can also store handler HandleAction(CStrID ActionID)! (template functor?)
		Events::PSub	Sub;

		CAction(): Enabled(true), Visible(true), IsSOAction(false) {}
		CAction(CStrID _ID, LPCSTR Name, int _Priority): ID(_ID), UIName(Name), Priority(_Priority), Enabled(true), Visible(true) {}

		LPCSTR	GetUIName() const { return UIName.IsValid() ? UIName.CStr() : ID.CStr(); }
		bool	operator <(const CAction& Other) const { return (Enabled && !Other.Enabled) || (Priority > Other.Priority); }
		bool	operator >(const CAction& Other) const { return (!Enabled && Other.Enabled) || (Priority < Other.Priority); }
	};

protected:

	Physics::PNodeAttrCollision	MousePickShape;
	nString						UIName;	//???use attribute?
	nString						UIDesc;	//???use attribute?
	nArray<CAction>				Actions;
	bool						Enabled;
	bool						TipVisible;
	bool						ReflectSOActions;

	virtual bool	InternalActivate();
	virtual void	InternalDeactivate();
	void			AddSOActions(CPropSmartObject& Prop);
	void			RemoveSOActions();

	bool			AddActionHandler(CStrID ID, LPCSTR UIName, Events::PEventHandler Handler, int Priority, bool IsSOAction = false);
	bool			ExecuteAction(Game::CEntity* pActorEnt, CAction& Action);
	bool			OnExecuteExploreAction(const Events::CEventBase& Event);
	bool			OnExecuteSelectAction(const Events::CEventBase& Event);
	bool			OnExecuteSmartObjAction(const Events::CEventBase& Event);
	CAction*		GetActionByID(CStrID ID);

	DECLARE_EVENT_HANDLER(ExposeSI, ExposeSI);
	DECLARE_EVENT_HANDLER(OnMouseEnter, OnMouseEnter);
	DECLARE_EVENT_HANDLER(OnMouseLeave, OnMouseLeave);
	DECLARE_EVENT_HANDLER(OnPropActivated, OnPropActivated);
	DECLARE_EVENT_HANDLER(OnPropDeactivating, OnPropDeactivating);
	DECLARE_EVENT_HANDLER(OnSOActionAvailabile, OnSOActionAvailabile);

public:

	enum { Priority_Default = 20, Priority_Top = 100 };

	CPropUIControl(): TipVisible(false), Enabled(true) { Actions.Flags.Clear(Array_KeepOrder); }

	void					Enable(bool SetEnabled);
	bool					IsEnabled() const { return Enabled; }
	void					SetUIName(const nString& NewName);
	void					ShowTip();
	void					HideTip();

	bool					AddActionHandler(CStrID ID, LPCSTR UIName, Events::CEventCallback Callback, int Priority = Priority_Default, bool IsSOAction = false);
	template<class T>
	bool					AddActionHandler(CStrID ID, LPCSTR UIName, T* Object, bool (T::*Callback)(const Events::CEventBase&), int Priority = Priority_Default, bool IsSOAction = false);
	bool					AddActionHandler(CStrID ID, LPCSTR UIName, LPCSTR ScriptFuncName, int Priority = Priority_Default, bool IsSOAction = false);
	void					RemoveActionHandler(CStrID ID);

	bool					ExecuteAction(Game::CEntity* pActorEnt, CStrID ID);
	bool					ExecuteDefaultAction(Game::CEntity* pActorEnt);
	void					ShowPopup(Game::CEntity* pActorEnt);

	const nArray<CAction>&	GetActions() const { return Actions; }
	void					EnableSmartObjReflection(bool Enable);
	bool					IsSmartObjReflectionEnabled() const { return ReflectSOActions; }
};

inline bool CPropUIControl::AddActionHandler(CStrID ID, LPCSTR UIName,
											 Events::CEventCallback Callback,
											 int Priority, bool IsSOAction)
{
	return AddActionHandler(ID, UIName, n_new(Events::CEventHandlerCallback)(Callback), Priority, IsSOAction);
}
//---------------------------------------------------------------------

template<class T>
inline bool CPropUIControl::AddActionHandler(CStrID ID, LPCSTR UIName, T* Object,
											 bool (T::*Callback)(const Events::CEventBase&),
											 int Priority, bool IsSOAction)
{
	return AddActionHandler(ID, UIName, n_new(Events::CEventHandlerMember<T>)(Object, Callback), Priority, IsSOAction);
}
//---------------------------------------------------------------------

inline CPropUIControl::CAction* CPropUIControl::GetActionByID(CStrID ID)
{
	for (nArray<CAction>::CIterator It = Actions.Begin(); It != Actions.End(); It++)
		if (It->ID == ID) return It;
	return NULL;
}
//---------------------------------------------------------------------

}

#endif