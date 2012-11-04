#ifndef N_OVERLOOKCAMERANODE_H
#define N_OVERLOOKCAMERANODE_H

#include "scene/nabstractcameranode.h"

// Extends nCameraNode.  Special camera, which look from up
// (C) 2006 Rong Zhou

class nOverlookCameraNode: public nAbstractCameraNode
{
public:

	virtual bool RenderCamera(const matrix44& modelWorldMatrix, const matrix44& viewMatrix, const matrix44& projectionMatrix);
};

#endif
