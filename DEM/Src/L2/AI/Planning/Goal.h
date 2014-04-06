#pragma once
#ifndef __DEM_L2_AI_GOAL_H__
#define __DEM_L2_AI_GOAL_H__

#include <Core/RefCounted.h>
#include <AI/Planning/WorldState.h>
#include <AI/ActorFwd.h>
#include <Data/Params.h>

// Goal describes what actor wants to change in the world or inside itself. Goal has relevance,
// actor can pursue only one or small number of goals at a time, relevance helps to choose them.
// NB: relevance base normally should be between 0.f (don't want) and 1.f (want more than anything)
// though it is only a convention and you may calculate relevances as you wish. Then this base is
// multiplied on the personality factor, so each character may have its own goal preferences.

namespace AI
{

class CGoal: public Core::CRefCounted
{
	__DeclareClassNoFactory;

protected:

	//???
	//int InterruptPriority

	float Relevance;
	float PersonalityFactor;	// Goal desirability multiplier, helps to tune personal preferences of actor
	//???need rel(*) & abs(+) parts for personality factor?

public:

	CGoal(): Relevance(0.f), PersonalityFactor(1.f) {}

	virtual void	Init(Data::PParams Params);
	virtual void	EvalRelevance(CActor* pActor) = 0;
	void			InvalidateRelevance() { Relevance = 0.f; }
	virtual void	GetDesiredProps(CWorldState& Dest) = 0;
	virtual bool	IsReplanningNeeded() const { FAIL; }
	virtual bool	IsSatisfied(/*actor*/) const { FAIL; }
	
	float			GetRelevance() const { return Relevance; }
};

typedef Ptr<CGoal> PGoal;

}

#endif