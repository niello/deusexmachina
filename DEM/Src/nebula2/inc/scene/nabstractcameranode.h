#ifndef N_ABSTRACTCAMERANODE_H
#define N_ABSTRACTCAMERANODE_H
//------------------------------------------------------------------------------
/**
    @class nAbstractCameraNode
    @ingroup Camera
    @brief A scene camera node can render the current scene from a different
    camera into a render target.

    (C) 2005 RadonLabs GmbH
*/

#include "scene/ntransformnode.h"
#include "renderpath/nrprendertarget.h"
#include "renderpath/nrenderpath2.h"

//------------------------------------------------------------------------------
class nAbstractCameraNode : public nTransformNode
{
public:
    /// constructor
    nAbstractCameraNode();
    /// destructor
    virtual ~nAbstractCameraNode();
    /// object persistency
    //virtual bool SaveCmds(nPersistServer* ps);
    /// provides a camera
    virtual bool HasCamera() const;
    /// render the scene from the provided camera
    virtual bool RenderCamera(const matrix44& modelWorldMatrix, const matrix44& viewMatrix, const matrix44& projectionMatrix);
    /// get the calculated view matrix
    const matrix44& GetViewMatrix() const;
    /// get the calculated projection matric
    const matrix44& GetProjectionMatrix();
    /// set the name of the render path section this camera should use
    void SetRenderPathSection(const nString& n);
    /// get the render path section name
    const nString& GetRenderPathSection() const;

protected:
    /// set the calculated view matrix
    void SetViewMatrix(const matrix44& m);
    /// set the calculated projection matrox
    void SetProjectionMatrix(const matrix44& m);

    matrix44 viewMatrix;
    matrix44 projMatrix;
    nString renderPathSection;
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nAbstractCameraNode::SetRenderPathSection(const nString& n)
{
    this->renderPathSection = n;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nString&
nAbstractCameraNode::GetRenderPathSection() const
{
    return this->renderPathSection;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAbstractCameraNode::SetViewMatrix(const matrix44& m)
{
    this->viewMatrix = m;
}

//------------------------------------------------------------------------------
/**
*/
inline
const matrix44&
nAbstractCameraNode::GetViewMatrix() const
{
    return this->viewMatrix;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAbstractCameraNode::SetProjectionMatrix(const matrix44& m)
{
    this->projMatrix = m;
}

//------------------------------------------------------------------------------
/**
*/
inline
const matrix44&
nAbstractCameraNode::GetProjectionMatrix()
{
    return this->projMatrix;
}

//------------------------------------------------------------------------------
#endif
