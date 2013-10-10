#pragma once
#ifndef __DEM_L2_AI_SMART_OBJ_ACTION_H__
#define __DEM_L2_AI_SMART_OBJ_ACTION_H__

#include "SmartObjActionTpl.h"
#include "Validator.h"
#include <AI/Planning/WorldStateSource.h>
#include <Data/StringID.h>

// Smart object action instance with per-SO data, based on CSmartObjActionTpl.

namespace Data
{
	typedef Ptr<class CParams> PParams;
}

namespace AI
{

//???store pSO inside?
class CSmartObjAction: public Core::CRefCounted
{
protected:

	const CSmartObjActionTpl&	Tpl;

public:

	bool				Enabled;
	bool				VisibleInUI;
	//LastActivationTime - for Timeout!
	int					FreeUserSlots;
	int					Resource; // How much uses left for this action before it becomes unavailable
	float				Progress; //???per-user? can't place into Action, it is destructed on abort plan.
	//???bool IsRunning;? actorID & progress-tracking should be separate for each actor performing action!

	//???planner worldstate source in validator or separate or in this? data-driven set of key-value pairs
	PValidator			ActivationValidator;
	PValidator			UpdateValidator;
	PWorldStateSource	Preconditions;

	CStrID				OnStartCmd;
	CStrID				OnDoneCmd;
	CStrID				OnEndCmd;
	CStrID				OnAbortCmd;

	CSmartObjAction(const CSmartObjActionTpl& _Tpl, Data::PParams Desc);

	bool IsValid(const CActor* pActor, const CPropSmartObject* pSO);

	bool EndOnDone() const { return Tpl.Flags.Is(CSmartObjActionTpl::END_ON_DONE); }
	bool ResetOnAbort() const { return Tpl.Flags.Is(CSmartObjActionTpl::RESET_ON_ABORT); }
	bool FaceObject() const { return Tpl.Flags.Is(CSmartObjActionTpl::FACE_OBJECT); }
	bool ActorRadiusMatters() const { return Tpl.Flags.Is(CSmartObjActionTpl::ACTOR_RADIUS_MATTERS); }

	const CSmartObjActionTpl& GetTpl() const { return Tpl; }
};

typedef Ptr<CSmartObjAction> PSmartObjAction;

inline bool CSmartObjAction::IsValid(const CActor* pActor, const CPropSmartObject* pSO)
{
	if (!Enabled || !Resource || !FreeUserSlots) FAIL;
	return ActivationValidator.IsValid() ? ActivationValidator->IsValid(pActor, pSO, this) : !!pActor;
}
//---------------------------------------------------------------------

}

#endif