#pragma once
#ifndef __DEM_L2_PROP_SMART_OBJECT_H__
#define __DEM_L2_PROP_SMART_OBJECT_H__

#include <Game/Property.h>
#include <Data/Params.h>
#include <AI/SmartObj/SmartObjAction.h>
#include <Data/Dictionary.h>

// Smart object provides set of actions that can be executed on it by actor either through command or AI.

// Dev info:
// Can make this class inherited from the AINode analog.

namespace Prop
{

class CPropSmartObject: public Game::CProperty
{
	__DeclareClass(CPropSmartObject);
	__DeclarePropertyStorage;

public:

	typedef CDict<CStrID, AI::PSmartObjAction> CActList;

protected:

	CStrID		TypeID;
	bool		Movable;
	CActList	Actions;
	CStrID		CurrState; //???empty state = disabled?

	virtual bool	InternalActivate();
	virtual void	InternalDeactivate();
	void			EnableSI(class CPropScriptable& Prop);
	void			DisableSI(class CPropScriptable& Prop);

	DECLARE_EVENT_HANDLER(OnPropActivated, OnPropActivated);
	DECLARE_EVENT_HANDLER(OnPropDeactivating, OnPropDeactivating);

public:

	bool				SetState(CStrID ID);
	bool				HasAction(CStrID ID) const { return Actions.FindIndex(ID) != INVALID_INDEX; }
	AI::PSmartObjAction	GetAction(CStrID ID) const;
	void				EnableAction(CStrID ActionID, bool Enable = true);
	bool				IsActionEnabled(CStrID ID) const;

	bool				CanDestinationChange() const { return Movable; }
	bool				GetDestination(CStrID ActionID, float ActorRadius, vector3& OutDest, float& OutMinDist, float& OutMaxDist);

	CStrID				GetTypeID() const { return TypeID; }
	const CActList&		GetActions() const { return Actions; }
	CStrID				GetCurrState() const { return CurrState; }
};

inline AI::PSmartObjAction CPropSmartObject::GetAction(CStrID ID) const
{
	int Idx = Actions.FindIndex(ID);
	return (Idx != INVALID_INDEX) ? Actions.ValueAt(Idx) : NULL;
}
//---------------------------------------------------------------------

inline bool CPropSmartObject::IsActionEnabled(CStrID ID) const
{
	int Idx = Actions.FindIndex(ID);
	return (Idx != INVALID_INDEX) && Actions.ValueAt(Idx)->Enabled;
}
//---------------------------------------------------------------------

}

#endif