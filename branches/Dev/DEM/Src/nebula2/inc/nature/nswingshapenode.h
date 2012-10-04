#ifndef N_SWINGSHAPENODE_H
#define N_SWINGSHAPENODE_H
//------------------------------------------------------------------------------
/**
    @class nSwingShapeNode
    @ingroup Nature

    Extends nShapeNode and provides the rotation matrix needed for
    the swing and leaves shaders.

    (C) 2003 RadonLabs GmbH
*/
#include "scene/nshapenode.h"
#include "variable/nvariable.h"

//------------------------------------------------------------------------------
class nSwingShapeNode : public nShapeNode
{
public:
    /// constructor
    nSwingShapeNode();
    /// destructor
    virtual ~nSwingShapeNode();
    /// save object to persistent stream
    //virtual bool SaveCmds(nPersistServer* ps);
    /// load resources
    virtual bool LoadResources();
    /// unload resources
    virtual void UnloadResources();
    /// get the mesh usage flags required by this shape node
    virtual int GetMeshUsage() const;
    /// perform pre-instancing rendering of shader
    virtual bool ApplyShader(nSceneServer* sceneServer);
    /// perform per-instance-rendering of shader
    virtual bool RenderShader(nSceneServer* sceneServer, nRenderContext* renderContext);
    /// set the max swing angle
    void SetSwingAngle(float f);
    /// get the swing angle
    float GetSwingAngle() const;
    /// set the swing time period
    void SetSwingTime(float t);
    /// get the swing time period
    float GetSwingTime() const;

private:
    /// compute a permuted swing angle
    float ComputeAngle(const vector3& pos, nTime time) const;

    nVariable::Handle timeVarHandle;
    nVariable::Handle windVarHandle;
    float swingAngle;
    float swingTime;
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nSwingShapeNode::SetSwingAngle(float f)
{
    this->swingAngle = f;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nSwingShapeNode::GetSwingAngle() const
{
    return this->swingAngle;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nSwingShapeNode::SetSwingTime(float t)
{
    this->swingTime = t;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nSwingShapeNode::GetSwingTime() const
{
    return this->swingTime;
}

//------------------------------------------------------------------------------
#endif
