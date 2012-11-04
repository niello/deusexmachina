#ifndef N_REFLECTIONCAMERANODE_H
#define N_REFLECTIONCAMERANODE_H
//------------------------------------------------------------------------------
/**
    @class nReflectionCameraNode
    @ingroup Camera
    @brief Extends nClippingCameraNode.  Special Clippingcamera which
    computes a Reflectionviewmatrix.

    author: matthias
    (C) 2004 RadonLabs GmbH
*/

#include "scene/nclippingcameranode.h"

//------------------------------------------------------------------------------
class nReflectionCameraNode : public nClippingCameraNode
{
public:

	virtual bool RenderCamera(const matrix44& ModelWorld, const matrix44& View, const matrix44& Proj);

protected:
    /// compute a reflection camera matrix
    matrix44 ComputeReflectionViewMatrix(const matrix44& viewMatrix, const matrix44& modelMatrix, const bool& isOnFrontSideOfPlane, const float& dist);
};
//------------------------------------------------------------------------------
#endif
