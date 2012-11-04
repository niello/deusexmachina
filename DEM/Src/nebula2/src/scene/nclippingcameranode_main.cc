//------------------------------------------------------------------------------
//  ncameranode_main.cc
//  (C) 2004 RadonLabs GmbH
//  author: matthias
//------------------------------------------------------------------------------
#include "scene/nclippingcameranode.h"
#include "mathlib/polar.h"
#include "kernel/nkernelserver.h"

nNebulaClass(nClippingCameraNode, "nabstractcameranode");

//------------------------------------------------------------------------------
/**
*/
bool
nClippingCameraNode::RenderCamera(const matrix44& modelWorldMatrix, const matrix44& viewMatrix, const matrix44& projectionMatrix)
{
    bool isOnFrontSideOfPlane = false;

    vector3 clipPlaneNormal = modelWorldMatrix.y_component();
    vector3 clipPlanePoint  = modelWorldMatrix.pos_component();

    // check on witch side the position of the camera is
    vector3 camPosWorldSpace;

    matrix44 _viewMatrix         = viewMatrix;
    _viewMatrix.pos_component()  = vector3::Zero;

    _viewMatrix.invert();
    _viewMatrix.mult(viewMatrix.pos_component(), camPosWorldSpace);

    // compute distance between camera and plane
    float distance  = modelWorldMatrix.y_component().x * (camPosWorldSpace.x + modelWorldMatrix.pos_component().x)
                     +modelWorldMatrix.y_component().y * (camPosWorldSpace.y + modelWorldMatrix.pos_component().y)
                     +modelWorldMatrix.y_component().z * (camPosWorldSpace.z + modelWorldMatrix.pos_component().z);

    clipPlaneNormal.norm();

    if (distance < 0.0f)
    {
        //front side of plane
        clipPlaneNormal = -clipPlaneNormal;

        isOnFrontSideOfPlane = true;
    }
    else
    {
        //back side of plane
        isOnFrontSideOfPlane = false;
    }

    // update projection Matrix
    this->projMatrix = this->ComputeProjectionMatrix(viewMatrix, projectionMatrix, clipPlaneNormal, clipPlanePoint - clipPlaneNormal*0.2f);

    // update viewMatrix
    this->viewMatrix = viewMatrix;

    return true;
}

//------------------------------------------------------------------------------
/**
*/
float
nClippingCameraNode::sgn(float a)
{
    if (a > 0.0f) return 1.0f;
    if (a < 0.0f) return -1.0f;
    return 0.0f;
}

//------------------------------------------------------------------------------
/**
http://www.gamedev.net/community/forums/topic.asp?topic_id=253039
*/
matrix44
nClippingCameraNode::ComputeProjectionMatrix(const matrix44& viewMatrix, const matrix44& projectionMatrix, const vector3& clipPlaneNormal, const vector3& clipPlanePoint)
{
    matrix44 _viewMatrix        = viewMatrix;
    matrix44 _projectionMatrix  = projectionMatrix;
    vector4  _clipPlane;

    //compute point on plane
    vector3 _pp;

    //bring point in viewspace
    _viewMatrix.mult(clipPlanePoint, _pp);

    //bring plane in viewspace
    _viewMatrix.mult(vector4(clipPlaneNormal.x, clipPlaneNormal.y, clipPlaneNormal.z, 0.0), _clipPlane);

    //compute new plane
    vector3 clipPlaneWorldNormal = vector3(_clipPlane.x, _clipPlane.y, _clipPlane.z);
    clipPlaneWorldNormal.norm();

    _clipPlane.w = -clipPlaneWorldNormal.dot(_pp);
    _clipPlane.x = clipPlaneWorldNormal.x;
    _clipPlane.y = clipPlaneWorldNormal.y;
    _clipPlane.z = clipPlaneWorldNormal.z;

    //compute real distance from objects to clipping plane
    _viewMatrix.y_component().norm();

    polar2 angle(_viewMatrix.y_component());

    float t = n_sin(N_PI/2.0f - angle.theta);

    //bring rotation over 0.1
    t = n_max(0.1f, n_abs(t));

    vector4 q;
    q.x = t*(this->sgn(_clipPlane.x)) / _projectionMatrix.m[0][0];
    q.y = t*(this->sgn(_clipPlane.y)) / _projectionMatrix.m[1][1];
    q.z = -1.0;
    q.w = t*(float)(1.0 + _projectionMatrix.m[2][2]) / _projectionMatrix.m[3][2];

    vector4 c = _clipPlane * (1.0f / (_clipPlane.x*q.x + _clipPlane.y*q.y + _clipPlane.z*q.z + _clipPlane.w*q.w));

    // Replace the third column of the projection matrix
    _projectionMatrix.m[0][2] = c.x;
    _projectionMatrix.m[1][2] = c.y;
    _projectionMatrix.m[2][2] = c.z;
    _projectionMatrix.m[3][2] = c.w;

    return _projectionMatrix;
}
