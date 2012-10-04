#pragma once
#ifndef __DEM_L3_AI_STIMULUS_SOUND_H__
#define __DEM_L3_AI_STIMULUS_SOUND_H__

#include <AI/Perception/Stimulus.h>

// Stimulus produced with the sound, short or continuous. Intensity depends on the volume level.

namespace AI
{

class CStimulusSound: public CStimulus
{
	DeclareRTTI;
	DeclareFactory(CStimulusSound);

protected:

public:

	//!!!sound type (danger, voice (by gender?), suspicious etc)!

};

RegisterFactory(CStimulusSound);

typedef Ptr<CStimulusSound> PStimulusSound;

}

#endif