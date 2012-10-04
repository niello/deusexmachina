#ifndef N_SUBDIVSHAPENODE_H
#define N_SUBDIVSHAPENODE_H
//------------------------------------------------------------------------------
/**
    @class nSubdivShapeNode
    @ingroup Scene

    @brief Takes an input mesh and renders a tesselated mesh (FIXME: for now
    a pointlist based on subdivision parameters.

    (C) 2003 RadonLabs GmbH
*/
#include "scene/nshapenode.h"
#include "gfx2/ndynamicmesh.h"

//------------------------------------------------------------------------------
class nSubdivShapeNode : public nShapeNode
{
public:
    /// constructor
    nSubdivShapeNode();
    /// destructor
    virtual ~nSubdivShapeNode();
    /// object persistency
    //virtual bool SaveCmds(nPersistServer* ps);
    /// perform pre-instancing rending of shader
    virtual bool ApplyShader(nSceneServer* sceneServer);
    /// perform pre-instancing rending of geometry
    virtual bool ApplyGeometry(nSceneServer* sceneServer);
    /// render geometry
    virtual bool RenderGeometry(nSceneServer* sceneServer, nRenderContext* renderContext);
    /// get the mesh usage flags required by this shape node
    virtual int GetMeshUsage() const;
    /// set the desired segment size
    void SetSegmentSize(float s);
    /// get segment size
    float GetSegmentSize() const;
    /// set max distance for subdivision (ignores triangles which are further away)
    void SetMaxDistance(float d);
    /// get max distance for subdivision
    float GetMaxDistance() const;

private:
    float segmentSize;
    float maxDistance;
    nDynamicMesh dynMesh;
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nSubdivShapeNode::SetSegmentSize(float s)
{
    n_assert(s > 0.0f);
    this->segmentSize = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nSubdivShapeNode::GetSegmentSize() const
{
    return this->segmentSize;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nSubdivShapeNode::SetMaxDistance(float d)
{
    n_assert(d > 0.0f);
    this->maxDistance = d;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nSubdivShapeNode::GetMaxDistance() const
{
    return this->maxDistance;
}

//------------------------------------------------------------------------------
#endif
