#pragma once
#include <Animation/PoseOutput.h>

// Transform output binding to a scene node hierarchy.
// Used for writing animation result into the scene graph.

namespace DEM::Anim
{

class CSkeleton : public IPoseOutput
{
public:

	// root node, required for bone map setup (binding setup)
	// port binding for bone map (or in IPoseOutput?)
};

}
