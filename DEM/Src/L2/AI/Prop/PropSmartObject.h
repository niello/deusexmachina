#pragma once
#ifndef __DEM_L2_PROP_SMART_OBJECT_H__
#define __DEM_L2_PROP_SMART_OBJECT_H__

#include <Game/Property.h>
#include <DB/AttrID.h>
#include <Data/Params.h>
#include <AI/SmartObj/SmartObjAction.h>
#include <util/ndictionary.h>

// Smart object provides set of actions that can be executed on it by actor either through command or AI.

// Dev info:
// Can make this class inherited from the AINode analog.

namespace Attr
{
	DeclareString(SmartObjDesc);	// Smart object description HRD file ID
};

namespace Properties
{
using namespace AI;

class CPropSmartObject: public Game::CProperty
{
	__DeclareClass(CPropSmartObject);
	__DeclarePropertyStorage;

public:

	typedef nDictionary<CStrID, PSmartObjAction> CActList;

protected:

	//bool Enabled; or empty state?
	CStrID		TypeID;
	CActList	Actions;
	CStrID		CurrState; //???empty state = disabled?

	DECLARE_EVENT_HANDLER(ExposeSI, ExposeSI);

public:

	virtual void	Activate();
	virtual void	Deactivate();
	virtual void	GetAttributes(nArray<DB::CAttrID>& Attrs);

	bool			SetState(CStrID ID);
	bool			HasAction(CStrID ID) const { return Actions.FindIndex(ID) != INVALID_INDEX; }
	PSmartObjAction	GetAction(CStrID ID) const;
	void			EnableAction(CStrID ActionID, bool Enable = true);
	bool			IsActionEnabled(CStrID ID) const;

	bool			GetDestination(CStrID ActionID, float ActorRadius, vector3& OutDest, float& OutMinDist, float& OutMaxDist);

	CStrID			GetTypeID() const { return TypeID; }
	const CActList&	GetActions() const { return Actions; }
	CStrID			GetCurrState() const { return CurrState; }
};

__RegisterClassInFactory(CPropSmartObject);

inline PSmartObjAction CPropSmartObject::GetAction(CStrID ID) const
{
	int Idx = Actions.FindIndex(ID);
	return (Idx != INVALID_INDEX) ? Actions.ValueAtIndex(Idx) : NULL;
}
//---------------------------------------------------------------------

inline bool CPropSmartObject::IsActionEnabled(CStrID ID) const
{
	//???check is in this state?
	int Idx = Actions.FindIndex(ID);
	return (Idx != INVALID_INDEX) && Actions.ValueAtIndex(Idx)->Enabled;
}
//---------------------------------------------------------------------

}

#endif