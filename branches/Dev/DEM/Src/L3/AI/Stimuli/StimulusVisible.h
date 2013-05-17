#pragma once
#ifndef __DEM_L3_AI_STIMULUS_VISIBLE_H__
#define __DEM_L3_AI_STIMULUS_VISIBLE_H__

#include <AI/Perception/Stimulus.h>

// Stimulus produced by any visible object. Intensity depends on transparence, brightness etc.

namespace AI
{

class CStimulusVisible: public CStimulus
{
	__DeclareClass(CStimulusVisible);

public:

};

typedef Ptr<CStimulusVisible> PStimulusVisible;

}

#endif