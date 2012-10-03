#pragma once
#ifndef __DEM_L3_AI_PERCEPTOR_SMART_OBJ_H__
#define __DEM_L3_AI_PERCEPTOR_SMART_OBJ_H__

#include <AI/Perception/Perceptor.h>

// Detects smart objects.

namespace AI
{

class CPerceptorIAO: public CPerceptor
{
	DeclareRTTI;
	DeclareFactory(CPerceptorIAO);

public:

	virtual void ProcessStimulus(CActor* pActor, CStimulus* pStimulus, float Confidence = 1.f);
};

RegisterFactory(CPerceptorIAO);

}

#endif