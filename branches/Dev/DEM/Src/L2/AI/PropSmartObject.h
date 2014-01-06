#pragma once
#ifndef __DEM_L2_PROP_SMART_OBJECT_H__
#define __DEM_L2_PROP_SMART_OBJECT_H__

#include <Game/Property.h>
#include <Data/Params.h>
#include <AI/SmartObj/SmartAction.h>
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

	struct CAction
	{
		const AI::CSmartAction*	pTpl;
		bool					Enabled;
		int						FreeUserSlots;

		bool IsValid(const AI::CActor* pActor, const CPropSmartObject* pSO) const { return pTpl && Enabled && FreeUserSlots && pTpl->IsValid(pActor, pSO); }
	};

	typedef CDict<CStrID, CAction> CActList;

protected:

	// FSM stuff, Tr is for Transition
	CStrID		CurrState;
	CStrID		TargetState;
	float		TrProgress;
	float		TrDuration;
	CStrID		TrActionID;
	bool		TrManualControl;
	CActList	Actions;

	// Game object stuff
	CStrID		TypeID;
	bool		Movable;

	virtual bool	InternalActivate();
	virtual void	InternalDeactivate();
	void			EnableSI(class CPropScriptable& Prop);
	void			DisableSI(class CPropScriptable& Prop);

	DECLARE_EVENT_HANDLER(OnPropsActivated, OnPropsActivated);
	DECLARE_EVENT_HANDLER(OnPropActivated, OnPropActivated);
	DECLARE_EVENT_HANDLER(OnPropDeactivating, OnPropDeactivating);
	DECLARE_EVENT_HANDLER(OnLevelSaving, OnLevelSaving);

	//!!!listen frame event to update automatic transition!
	//subscribe only when automatic transition is active

public:

	bool				SetState(CStrID ID, CStrID ActionID, bool ManualControl = false);
	void				SetTransitionDuration(float Time);
	void				SetTransitionProgress(float Time);
	void				StopTransition();
	void				AbortTransition();
	bool				IsInTransition() const { return CurrState != TargetState; }
	CStrID				GetCurrState() const { return CurrState; }
	CStrID				GetTargetState() const { return TargetState; }
	float				GetTransitionDuration() const { return TrDuration; }
	float				GetTransitionProgress() const { return TrProgress; } //???return 0 when no transition?
	CStrID				GetTransitionActionID() const { return TrActionID; }

	bool				HasAction(CStrID ID) const { return Actions.FindIndex(ID) != INVALID_INDEX; }
	CAction*			GetAction(CStrID ID);
	const CActList&		GetActions() const { return Actions; }
	void				EnableAction(CStrID ActionID, bool Enable = true);
	bool				IsActionEnabled(CStrID ID) const;

	CStrID				GetTypeID() const { return TypeID; }
	bool				IsMovable() const { return Movable; }
	bool				GetDestinationParams(CStrID ActionID, float ActorRadius, vector3& OutOffset, float& OutMinDist, float& OutMaxDist);
};

inline CPropSmartObject::CAction* CPropSmartObject::GetAction(CStrID ID)
{
	int Idx = Actions.FindIndex(ID);
	return (Idx != INVALID_INDEX) ? &Actions.ValueAt(Idx) : NULL;
}
//---------------------------------------------------------------------

inline bool CPropSmartObject::IsActionEnabled(CStrID ID) const
{
	int Idx = Actions.FindIndex(ID);
	return (Idx != INVALID_INDEX) && Actions.ValueAt(Idx).Enabled;
}
//---------------------------------------------------------------------

}

#endif