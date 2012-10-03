//------------------------------------------------------------------------------
//  noverlookcameranode_main.cc
//  (C) 2006 Rong Zhou
//------------------------------------------------------------------------------
#include "scene/noverlookcameranode.h"
#include "mathlib/polar.h"

nNebulaClass(nOverlookCameraNode, "nabstractcameranode");

//------------------------------------------------------------------------------
/**
*/
nOverlookCameraNode::nOverlookCameraNode()
{
    //empty
}

//------------------------------------------------------------------------------
/**
*/
nOverlookCameraNode::~nOverlookCameraNode()
{
    //empty
}

//------------------------------------------------------------------------------
/**
*/
matrix44
nOverlookCameraNode::ComputeViewMatrix(const vector3& cameraPosition)
{
    matrix44 viewMatrix;
    viewMatrix.rotate_x(N_PI*0.5f);

    vector3 _cameraPosition;
    viewMatrix.mult(-cameraPosition, _cameraPosition);
    viewMatrix.pos_component() = _cameraPosition;

    return viewMatrix;
}


//------------------------------------------------------------------------------
/**
*/
bool
nOverlookCameraNode::RenderCamera(const matrix44& modelWorldMatrix, const matrix44& /*viewMatrix*/, const matrix44& projectionMatrix)
{
    // projectionmatrix
    this->projMatrix = projectionMatrix;

    // viewmatrix
    this->viewMatrix = this->ComputeViewMatrix(modelWorldMatrix.pos_component());

    return true;
}
