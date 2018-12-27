#pragma once
#ifndef __DEM_L2_AI_PERCEPTOR_H__
#define __DEM_L2_AI_PERCEPTOR_H__

#include <Core/Object.h>
#include <Data/Params.h>
#include <AI/ActorFwd.h>

// When a sensor is a passive stimulus collector, a perceptor is an active brain part, that
// interprets sensory information and produces memory facts about some kinds of symbols, which
// in turn are operable by decision making and planning mechanisms.

namespace AI
{
class CSensor;
class CStimulus;

class CPerceptor: public Core::CObject
{
	__DeclareClassNoFactory;

protected:

	// Enabled

	//???or only in sensors?
	// NextUpdateTime
	// UpdateRate

public:

	virtual void Init(const Data::CParams& Desc) {}
	virtual void ProcessStimulus(CActor* pActor, CStimulus* pStimulus, float Confidence = 1.f) = 0;
};

typedef Ptr<CPerceptor> PPerceptor;

}

#endif