#pragma once
#include <Game/Property.h>
#include <Data/Params.h>
#include <AI/ActorFwd.h>
#include <Data/Dictionary.h>
#include <Math/Vector3.h>
#include <DetourNavMesh.h> // for dtPolyRef

// Smart object provides set of actions that can be executed on it by actor either through command or AI.

// Dev info:
// Can make this class inherited from the AINode analog.

namespace AI
{
	class CSmartAction;
}

namespace Prop
{

class CPropSmartObject: public Game::CProperty
{
	FACTORY_CLASS_DECL;
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
		float	Offset;
		float	Speed;
		float	Weight;
		//???priority, fadein, fadeout?
	};

	// FSM stuff, Tr is for Transition
	CActionList			Actions;
	CArray<CAnimInfo>	Anims;
	CDict<CStrID, int>	ActionAnimIndices;
	CDict<CStrID, int>	StateAnimIndices;
	CStrID				CurrState;
	CStrID				TargetState;
	float				TrProgress;
	float				TrDuration;
	CStrID				TrActionID;
	bool				TrManualControl;
	UPTR				AnimTaskID = INVALID_INDEX;
	const CAnimInfo*	pCurrAnimInfo = nullptr;

	// Game object stuff
	CStrID				TypeID;

	virtual bool	InternalActivate();
	virtual void	InternalDeactivate();
	void			EnableSI(class CPropScriptable& Prop);
	void			DisableSI(class CPropScriptable& Prop);
	void			InitAnimation(Data::PParams Desc, class CPropAnimation& Prop);

	void			CompleteTransition();
	void			SwitchAnimation(const CAnimInfo* pAnimInfo);
	void			UpdateAnimationCursor();

	DECLARE_EVENT_HANDLER(OnPropsActivated, OnPropsActivated);
	DECLARE_EVENT_HANDLER(OnPropActivated, OnPropActivated);
	DECLARE_EVENT_HANDLER(OnPropDeactivating, OnPropDeactivating);
	DECLARE_EVENT_HANDLER(OnLevelSaving, OnLevelSaving);
	DECLARE_EVENT_HANDLER(OnBeginFrame, OnBeginFrame);

public:

	CPropSmartObject();
	virtual ~CPropSmartObject();

	bool				SetState(CStrID StateID, CStrID ActionID = CStrID::Empty, float TransitionDuration = -1.f, bool ManualControl = false);
	void				SetTransitionDuration(float Time);
	void				SetTransitionProgress(float Time);
	void				StopTransition();
	void				AbortTransition(float Duration = 0.f);
	bool				IsInTransition() const { return CurrState != TargetState; }
	CStrID				GetCurrState() const { return CurrState; }
	CStrID				GetTargetState() const { return TargetState; }
	float				GetTransitionDuration() const { return TrDuration; }
	float				GetTransitionProgress() const { return TrProgress; } //IsInTransition() ? TrProgress : 0.f; }
	CStrID				GetTransitionActionID() const { return TrActionID; }

	bool				HasAction(CStrID ID) const { return Actions.FindIndex(ID) != INVALID_INDEX; }
	CAction*			GetAction(CStrID ID);
	const CAction*		GetAction(CStrID ID) const;
	const CActionList&	GetActions() const { return Actions; }
	void				EnableAction(CStrID ID, bool Enable = true);
	bool				IsActionEnabled(CStrID ID) const;
	bool				IsActionAvailable(CStrID ID, const AI::CActor* pActor) const;

	CStrID				GetTypeID() const { return TypeID; }
	bool				GetRequiredActorPosition(CStrID ActionID, const AI::CActor* pActor, vector3& OutPos, CArray<dtPolyRef>* pNavCache = nullptr, bool UpdateCache = false);
	bool				GetRequiredActorFacing(CStrID ActionID, const AI::CActor* pActor, vector3& OutFaceDir);
};

}
