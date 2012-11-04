#ifndef N_CLIPPINGCAMERANODE_H
#define N_CLIPPINGCAMERANODE_H
//------------------------------------------------------------------------------
/**
    @class nClippingCameraNode
    @ingroup Camera
    @brief Extends nCameraNode.  Special camera, which computes a clipping
    plane in the Projectionmatrix.

    author: matthias
    (C) 2004 RadonLabs GmbH
*/
#include "scene/nabstractcameranode.h"

//------------------------------------------------------------------------------
class nClippingCameraNode : public nAbstractCameraNode
{
public:

	virtual bool RenderCamera(const matrix44& modelWorldMatrix, const matrix44& viewMatrix, const matrix44& projectionMatrix);

protected:
    /// compute a reflection projection matrix with clipping
    matrix44 ComputeProjectionMatrix(const matrix44& viewMatrix, const matrix44& projectionMatrix, const vector3& clipPlaneNormal, const vector3& clipPlanePoint);

private:
    /// help function
    float sgn(float a);
};
//------------------------------------------------------------------------------
#endif
