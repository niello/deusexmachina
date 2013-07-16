#pragma once
#ifndef __DEM_L2_PROP_AI_HINTS_H__
#define __DEM_L2_PROP_AI_HINTS_H__

#include <Game/Property.h>
#include <AI/Perception/Stimulus.h>

// This property manages all the AI stimuli and hints produced by a single game entity.
// Speaking figuratively, this property makes the entity smell, sound and be visible for the actors.

namespace Prop
{
using namespace AI;

class CPropAIHints: public Game::CProperty
{
	__DeclareClass(CPropAIHints);
	__DeclarePropertyStorage;

protected:

	struct CRecord
	{
		PStimulus		Stimulus;
		CStimulusNode	QTNode;
	};

	CDict<CStrID, CRecord> Hints;

	virtual bool InternalActivate();
	virtual void InternalDeactivate();

	DECLARE_EVENT_HANDLER(OnPropsActivated, OnPropsActivated);
	DECLARE_EVENT_HANDLER(OnRenderDebug, OnRenderDebug);
	DECLARE_EVENT_HANDLER(ExposeSI, ExposeSI);
	DECLARE_EVENT_HANDLER(UpdateTransform, OnUpdateTransform);

public:

	//CPropAIHints();
	//virtual ~CPropAIHints();

	//CreateStimulus(Name, Type, ExpiratioCTime, Confidence, ...)
	void EnableStimulus(CStrID Name, bool Enable);
	//EnableStimulus(Type, bool) - Type as RTTI?
	//SetStimulusConfidence(Name/Type, Confidence)
	//SetStimulusExpiratioCTime(Name, Time)
	//DestroyStimulus(Name)
};

}

#endif