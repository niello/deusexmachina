#pragma once
#include <AI/Perception/Perceptor.h>

// Processes visual and tactile info and generates obstacles to avoid
// Mb add danger level checking here, so we can create bigger obstacles for more dangerous objects.
// Obstacles are respected only by steering, but may be they should also affect navmesh poly
// traversal cost for the current actor.

namespace AI
{

class CPerceptorObstacle: public CPerceptor
{
	FACTORY_CLASS_DECL;

public:

	virtual void ProcessStimulus(CActor* pActor, CStimulus* pStimulus, float Confidence = 1.f);
};

}
