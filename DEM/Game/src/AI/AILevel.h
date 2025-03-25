#pragma once
#include <Core/Object.h>
#include <AI/Perception.h>

// AI level is an abstract space, like scene or CPhysicsLevel, that contains stimuli,
// AI hints and other AI-related world info. Also AILevel is intended to serve as a
// navigation manager in the future.

namespace Debug
{
	class CDebugDraw;
}

namespace DEM::AI
{
using PAILevel = Ptr<class CAILevel>;

class CAILevel : public Core::CObject
{
protected:

	std::vector<CStimulusEvent> _StimulusEvents; // pending for processing in the next AI frame

public:

	void RenderDebug(Debug::CDebugDraw& DebugDraw);
};
//---------------------------------------------------------------------

}
