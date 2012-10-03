#ifndef N_OVERLOOKCAMERANODE_H
#define N_OVERLOOKCAMERANODE_H
//------------------------------------------------------------------------------
/**
    @class nOverlookCameraNode
    @ingroup Camera
    @brief Extends nCameraNode.  Special camera, which look from up

    (C) 2006 Rong Zhou
*/
#include "scene/nabstractcameranode.h"

//------------------------------------------------------------------------------
class nOverlookCameraNode : public nAbstractCameraNode
{
public:
    /// constructor
    nOverlookCameraNode();
    /// destructor
    virtual ~nOverlookCameraNode();

    /// function witch updates the camera
    virtual bool RenderCamera(const matrix44& modelWorldMatrix, const matrix44& viewMatrix, const matrix44& projectionMatrix);

protected:
    /// compute the viewmatrix
    matrix44 ComputeViewMatrix(const vector3& cameraPosition);

};
//------------------------------------------------------------------------------
#endif
