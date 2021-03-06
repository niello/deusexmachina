#pragma once
#include <Game/Property.h>
#include <Physics/CollisionAttribute.h>
#include <Events/EventHandler.h>
#include <Data/Array.h>
#include <Data/String.h>

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
class CGameLevelView;

class CPropUIControl: public Game::CProperty
{
	FACTORY_CLASS_DECL;
	__DeclarePropertyStorage;

public:

	struct CAction
	{
		CStrID			ID;
		CStrID			EventID;	// cached
		CString			UIName;		//???use CString to reduce size of struct?
		short			Priority;	// The higher is value the closer an action to the top of the list
		bool			Enabled = true;	//!!!add methods to control it!
		bool			Visible = true;
		bool			IsSOAction = true;
		//???picture, or associate with ID? if so, UIName & Priority can also be associated with ID

		//!!!can also store handler HandleAction(CStrID ActionID)! (template functor?)
		Events::PSub	Sub;

		CAction();
		CAction(CStrID _ID, const char* Name, int _Priority);

		const char*	GetUIName() const { return UIName.IsValid() ? UIName.CStr() : ID.CStr(); }
		bool	operator <(const CAction& Other) const { return (Enabled != Other.Enabled) ? Enabled : (Priority > Other.Priority); }
		bool	operator >(const CAction& Other) const { return (Enabled != Other.Enabled) ? Other.Enabled : (Priority < Other.Priority); }
		bool	operator ==(const CAction& Other) const { return ID == Other.ID; }
	};

protected:

	Physics::PCollisionAttribute	MousePickShape;
	CString						UIName;	//???use attribute?
	CString						UIDesc;	//???use attribute?
	CArray<CAction>				Actions;
	bool						Enabled = true;
	bool						TipVisible = false;
	bool						ReflectSOActions;

	virtual bool	InternalActivate();
	virtual void	InternalDeactivate();
	void			AddSOActions(CPropSmartObject& Prop);
	void			RemoveSOActions();
	void			EnableSI(class CPropScriptable& Prop);
	void			DisableSI(class CPropScriptable& Prop);

	bool			AddActionHandler(CStrID ID, const char* UIName, Events::PEventHandler&& Handler, int Priority, bool IsSOAction = false);
	bool			ExecuteAction(Game::CEntity* pActorEnt, CAction& Action);
	bool			OnExecuteExploreAction(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool			OnExecuteSelectAction(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool			OnExecuteSmartObjAction(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	CAction*		GetActionByID(CStrID ID);

	DECLARE_EVENT_HANDLER(OnLevelSaving, OnLevelSaving);
	DECLARE_EVENT_HANDLER(OnMouseEnter, OnMouseEnter);
	DECLARE_EVENT_HANDLER(OnMouseLeave, OnMouseLeave);
	DECLARE_EVENT_HANDLER(OnPropActivated, OnPropActivated);
	DECLARE_EVENT_HANDLER(OnPropDeactivating, OnPropDeactivating);
	DECLARE_EVENT_HANDLER(OnSOActionAvailabile, OnSOActionAvailabile);

public:

	enum { Priority_Default = 20, Priority_Top = 100 };

	CPropUIControl();

	void					Enable(bool SetEnabled);
	bool					IsEnabled() const { return Enabled; }
	void					SetUIName(const char* pNewName);
	void					ShowTip(CGameLevelView* pView);
	void					HideTip();

	bool					AddActionHandler(CStrID ID, const char* UIName, Events::CEventCallback Callback, int Priority = Priority_Default, bool IsSOAction = false);
	template<class T>
	bool					AddActionHandler(CStrID ID, const char* UIName, T* Object, bool (T::*Callback)(Events::CEventDispatcher*, const Events::CEventBase&), int Priority = Priority_Default, bool IsSOAction = false);
	bool					AddActionHandler(CStrID ID, const char* UIName, const char* ScriptFuncName, int Priority = Priority_Default, bool IsSOAction = false);
	void					RemoveActionHandler(CStrID ID);

	bool					ExecuteAction(Game::CEntity* pActorEnt, CStrID ID);
	bool					ExecuteDefaultAction(Game::CEntity* pActorEnt);
	void					ShowPopup(Game::CEntity* pActorEnt);

	const CArray<CAction>&	GetActions() const { return Actions; }
	void					EnableSmartObjReflection(bool Enable);
	bool					IsSmartObjReflectionEnabled() const { return ReflectSOActions; }
};

inline bool CPropUIControl::AddActionHandler(CStrID ID, const char* UIName,
											 Events::CEventCallback Callback,
											 int Priority, bool IsSOAction)
{
	return AddActionHandler(ID, UIName, n_new(Events::CEventHandlerCallback)(Callback), Priority, IsSOAction);
}
//---------------------------------------------------------------------

template<class T>
inline bool CPropUIControl::AddActionHandler(CStrID ID, const char* UIName, T* Object,
											 bool (T::*Callback)(Events::CEventDispatcher*, const Events::CEventBase&),
											 int Priority, bool IsSOAction)
{
	return AddActionHandler(ID, UIName, n_new(Events::CEventHandlerMember<T>)(Object, Callback), Priority, IsSOAction);
}
//---------------------------------------------------------------------

inline CPropUIControl::CAction* CPropUIControl::GetActionByID(CStrID ID)
{
	for (CArray<CAction>::CIterator It = Actions.Begin(); It != Actions.End(); ++It)
		if (It->ID == ID) return It;
	return nullptr;
}
//---------------------------------------------------------------------

}
