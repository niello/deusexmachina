//------------------------------------------------------------------------------
//  ncameranode_main.cc
//  (C) 2004 RadonLabs GmbH
//  author: matthias
//------------------------------------------------------------------------------
#include "scene/nreflectioncameranode.h"
#include "scene/nrendercontext.h"
#include "gfx2/ngfxserver2.h"
#include "mathlib/polar.h"

nNebulaClass(nReflectionCameraNode, "nclippingcameranode");

//------------------------------------------------------------------------------
/**
*/
bool
nReflectionCameraNode::RenderCamera(const matrix44& modelWorldMatrix, const matrix44& viewMatrix, const matrix44& projectionMatrix)
{
    bool isOnFrontSideOfPlane = false;

    vector3 clipPlaneNormal = modelWorldMatrix.y_component();
    vector3 clipPlanePoint  = modelWorldMatrix.pos_component();

    // check on witch side the position of the camera is
    vector3 camPosWorldSpace;

    matrix44 _modelWorldMatrix = modelWorldMatrix;

    matrix44 _viewMatrix         = viewMatrix;
    _viewMatrix.pos_component()  = vector3(0.0, 0.0, 0.0);

    _viewMatrix.invert();
    _viewMatrix.mult(viewMatrix.pos_component(), camPosWorldSpace);

    _modelWorldMatrix.y_component().norm();

    // compute distance between camera and plane
    float distance  = _modelWorldMatrix.y_component().x * (camPosWorldSpace.x + modelWorldMatrix.pos_component().x)
                     +_modelWorldMatrix.y_component().y * (camPosWorldSpace.y + modelWorldMatrix.pos_component().y)
                     +_modelWorldMatrix.y_component().z * (camPosWorldSpace.z + modelWorldMatrix.pos_component().z);

    if (distance < 0.0) isOnFrontSideOfPlane = true;
    else
    {
        clipPlaneNormal = -clipPlaneNormal;
        isOnFrontSideOfPlane = false;
    }

    //viewMatrix
    this->viewMatrix = this->ComputeReflectionViewMatrix(viewMatrix, modelWorldMatrix, isOnFrontSideOfPlane, distance);

    this->projMatrix = this->ComputeProjectionMatrix(this->viewMatrix, projectionMatrix, clipPlaneNormal, clipPlanePoint);

    return true;
}

//------------------------------------------------------------------------------
/**
*/
matrix44
nReflectionCameraNode::ComputeReflectionViewMatrix(const matrix44& viewMatrix, const matrix44& modelMatrix, const bool& isOnFrontSideOfPlane, const float& dist)
{
    matrix44 reflectionMatrix           = viewMatrix;
    reflectionMatrix.pos_component()    = vector3(0.0, 0.0, 0.0);
    polar2 angles;

    // compute rotation
    reflectionMatrix.y_component().norm();
    angles.set(reflectionMatrix.y_component());

    //if (reflectionMatrix.z_component().y < 0)
    float test = polar2(reflectionMatrix.z_component()).rho;

	float Angle = 2.f * angles.theta;
    reflectionMatrix.rotate_x(viewMatrix.y_component().z > 0 ? -Angle : Angle);

    // compute position
    vector3 posVec = vector3(0.0, -modelMatrix.pos_component().y, 0.0);

    posVec.rotate(vector3(1.0, 0.0, 0.0), angles.theta);
    posVec.rotate(vector3(0.0, 1.0, 0.0), angles.rho);
    reflectionMatrix.pos_component() = viewMatrix.pos_component() - vector3(2 * posVec.x, 2 * posVec.y, 2 * posVec.z);
    reflectionMatrix.pos_component() = vector3(reflectionMatrix.pos_component().x, -reflectionMatrix.pos_component().y, reflectionMatrix.pos_component().z);

    return reflectionMatrix;
}
