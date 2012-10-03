//------------------------------------------------------------------------------
//  ncameranode_main.cc
//  (C) 2004 RadonLabs GmbH
//  author: matthias
//------------------------------------------------------------------------------
#include "scene/nabstractcameranode.h"

nNebulaClass(nAbstractCameraNode, "ntransformnode");

//------------------------------------------------------------------------------
/**
*/
nAbstractCameraNode::nAbstractCameraNode()
{
    //empty
}

//------------------------------------------------------------------------------
/**
*/
nAbstractCameraNode::~nAbstractCameraNode()
{
    // unload the resources
    this->UnloadResources();
}

//------------------------------------------------------------------------------
/**
*/
bool
nAbstractCameraNode::HasCamera() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
nAbstractCameraNode::RenderCamera(const matrix44& /*modelWorldMatrix*/, const matrix44& /*viewMatrix*/, const matrix44& /*projectionMatrix*/)
{
    return false;
}
