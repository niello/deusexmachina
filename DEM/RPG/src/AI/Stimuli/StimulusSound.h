#pragma once
#include <AI/Perception/Stimulus.h>

// Stimulus produced with the sound, short or continuous. Intensity depends on the volume level.

namespace AI
{

class CStimulusSound: public CStimulus
{
	FACTORY_CLASS_DECL;

public:

	//!!!sound type (danger, voice (by gender?), suspicious etc)!

};

typedef Ptr<CStimulusSound> PStimulusSound;

}
