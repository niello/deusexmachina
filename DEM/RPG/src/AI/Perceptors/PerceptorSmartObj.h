#pragma once
#include <AI/Perception/Perceptor.h>

// Detects smart objects.

namespace AI
{

class CPerceptorSmartObj: public CPerceptor
{
	FACTORY_CLASS_DECL;

public:

	virtual void ProcessStimulus(CActor* pActor, CStimulus* pStimulus, float Confidence = 1.f);
};

}
