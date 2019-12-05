#pragma once
#include <AI/Perception/Stimulus.h>

// Stimulus produced by any visible object. Intensity depends on transparence, brightness etc.

namespace AI
{

class CStimulusVisible: public CStimulus
{
	FACTORY_CLASS_DECL;

public:

};

typedef Ptr<CStimulusVisible> PStimulusVisible;

}
