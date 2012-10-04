#pragma once
#ifndef __DEM_L2_AI_ACTION_TPL_H__
#define __DEM_L2_AI_ACTION_TPL_H__

#include <AI/Planning/WorldState.h>
#include <AI/ActorFwd.h>
#include <Data/Params.h>

// Action template is a base block of behaviour. Action manages actor state and sends requests
// to various subsystems like navigation, movement etc. Template is a singleton used for planning,
// actions are instantiated for the valid plan, receiving context data from it.

namespace AI
{
typedef Ptr<class CAction> PAction;

//???need refcount & factory? can instantiate manually
class CActionTpl: public Core::CRefCounted
{
	DeclareRTTI;

protected:

	//CWorldState	WSPreconditions;
	CWorldState	WSEffects;

	int			Precedence;
	int			Cost;

public:

	virtual void		Init(PParams Params);
	virtual bool		ValidateContextPreconditions(CActor* pActor, const CWorldState& WSGoal) { OK; }
	virtual PAction		CreateInstance(const CWorldState& Context) const = 0;

	virtual bool		GetPreconditions(CActor* pActor, CWorldState& WS, const CWorldState& WSGoal) const { FAIL; } // No preconditions, can avoid operations on them
	//virtual void		GetEffects(CWorldState& WS) const = 0; - will be needed if effects will be dynamic too
	const CWorldState&	GetEffects() const { return WSEffects; }
	int					GetPrecedence() const { return Precedence; }
	int					GetCost() const { return Cost; }
};

typedef Ptr<CActionTpl> PActionTpl;

}

#endif