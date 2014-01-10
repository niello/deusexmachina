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
	};

	typedef CDict<CStrID, CAction> CActionList;

protected:

	struct CAnimInfo
	{
		CStrID	ClipID;
		float	Duration; // Cached value
		bool	Loop;
		float	Offset; //!!!due to RelOffset init CAnimInfo or at least Offset when CPropAnimation is activated!
		float	Speed;
		float	Weight;
		//???priority, fadein, fadeout?
	};

	// FSM stuff, Tr is for Transition
	CActionList					Actions;
	CDict<CStrID, CAnimInfo>	ActionAnims; //???one collection and associations? state1(anim1)->state2(anim1) will
	CDict<CStrID, CAnimInfo>	StateAnims;	 //lead to no switch and it is very cool. Also data duplication will be avoided.
	CStrID						CurrState;
	CStrID						TargetState;
	float						TrProgress;
	float						TrDuration;
	CStrID						TrActionID;
	bool						TrManualControl;
	DWORD						AnimTaskID;
	const CAnimInfo*			pCurrAnimInfo;

	//!!!store animation mapping!

	// Game object stuff
	CStrID		TypeID;
	bool		Movable;

	virtual bool	InternalActivate();
	virtual void	InternalDeactivate();
	void			EnableSI(class CPropScriptable& Prop);
	void			DisableSI(class CPropScriptable& Prop);
	void			InitAnimation(Data::PParams Desc, class CPropAnimation& Prop);
	void			FillAnimationInfo(CAnimInfo& AnimInfo, const Data::CParams& Desc, class CPropAnimation& Prop);

	void			CompleteTransition();
	void			SwitchAnimation(const CAnimInfo* pAnimInfo);
	void			UpdateAnimationCursor();

	DECLARE_EVENT_HANDLER(OnPropsActivated, OnPropsActivated);
	DECLARE_EVENT_HANDLER(OnPropActivated, OnPropActivated);
	DECLARE_EVENT_HANDLER(OnPropDeactivating, OnPropDeactivating);
	DECLARE_EVENT_HANDLER(OnLevelSaving, OnLevelSaving);
	DECLARE_EVENT_HANDLER(OnBeginFrame, OnBeginFrame);

public:

	CPropSmartObject(): Actions(1, 2), ActionAnims(0, 2), StateAnims(0, 2), AnimTaskID(INVALID_INDEX), pCurrAnimInfo(NULL) {}

	bool				SetState(CStrID StateID, CStrID ActionID = CStrID::Empty, float TransitionDuration = -1.f, bool ManualControl = false);
	void				SetTransitionDuration(float Time);
	void				SetTransitionProgress(float Time);
	void				StopTransition() { UNSUBSCRIBE_EVENT(OnBeginFrame); }
	void				AbortTransition(float Duration = 0.f);
	bool				IsInTransition() const { return CurrState != TargetState; }
	CStrID				GetCurrState() const { return CurrState; }
	CStrID				GetTargetState() const { return TargetState; }
	float				GetTransitionDuration() const { return TrDuration; }
	float				GetTransitionProgress() const { return TrProgress; } //IsInTransition() ? TrProgress : 0.f; }
	CStrID				GetTransitionActionID() const { return TrActionID; }

	bool				HasAction(CStrID ID) const { return Actions.FindIndex(ID) != INVALID_INDEX; }
	CAction*			GetAction(CStrID ID);
	const CActionList&	GetActions() const { return Actions; }
	void				EnableAction(CStrID ID, bool Enable = true);
	bool				IsActionEnabled(CStrID ID) const;
	bool				IsActionAvailable(CStrID ID, const AI::CActor* pActor) const;

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
	if (Idx == INVALID_INDEX) FAIL;
	const CAction& Action = Actions.ValueAt(Idx);
	return Action.Enabled && Action.pTpl;
}
//---------------------------------------------------------------------

}

#endif