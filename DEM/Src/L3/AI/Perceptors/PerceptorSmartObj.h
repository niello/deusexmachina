#pragma once
#ifndef __DEM_L3_AI_PERCEPTOR_SMART_OBJ_H__
#define __DEM_L3_AI_PERCEPTOR_SMART_OBJ_H__

#include <AI/Perception/Perceptor.h>

// Detects smart objects.

namespace AI
{

class CPerceptorSmartObj: public CPerceptor
{
	__DeclareClass(CPerceptorSmartObj);

public:

	virtual void ProcessStimulus(CActor* pActor, CStimulus* pStimulus, float Confidence = 1.f);
};

__RegisterClassInFactory(CPerceptorSmartObj);

}

#endif