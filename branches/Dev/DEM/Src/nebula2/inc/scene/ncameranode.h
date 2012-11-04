#ifndef N_CAMERANODE_H
#define N_CAMERANODE_H

#include "scene/nabstractcameranode.h"

// Extends nAbstractCameraNode. Is a fixed camera with no intelligence.
// (C) 2004 RadonLabs GmbH

class nCameraNode: public nAbstractCameraNode
{
public:

	virtual bool RenderCamera(const matrix44& ModelWorld, const matrix44& /*View*/, const matrix44& Proj);
};

#endif
