#pragma once
#ifndef __DEM_L3_AI_ACTION_WANDER_H__
#define __DEM_L3_AI_ACTION_WANDER_H__

#include <AI/Behaviour/Action.h> //Sequence.h>

// Wander action switches actor between moving in certain radius and standing still,
// sometimes facing random directions.

namespace AI
{

class CActionWander: public CAction //Sequence
{
	DeclareRTTI;
	DeclareFactory(CActionWander);

protected:

	vector2	InitialPos;
	float	NextActSelectionTime;
	PAction	CurrAction;

	bool SelectAction(CActor* pActor);

public:

	//void				Init(action probabilities, timings, wander radius etc);

	virtual bool		Activate(CActor* pActor);
	virtual EExecStatus	Update(CActor* pActor);
	virtual void		Deactivate(CActor* pActor);
	//
	//virtual bool		IsValid() const { return ppCurrChild && (*ppCurrChild)->IsValid(); }
};

RegisterFactory(CActionWander);

typedef Ptr<CActionWander> PActionWander;

}

#endif