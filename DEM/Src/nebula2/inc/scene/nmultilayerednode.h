#ifndef N_MULTILAYEREDNODE_H
#define N_MULTILAYEREDNODE_H
//------------------------------------------------------------------------------
/**
    @class nMultiLayeredNode
    @ingroup Scene

    (C) 2005 RadonLabs GmbH
*/
#include "scene/nshapenode.h"

class nRenderContext;
//------------------------------------------------------------------------------
class nMultiLayeredNode : public nShapeNode
{
public:
    /// constructor
    nMultiLayeredNode();
    /// destructor
    virtual ~nMultiLayeredNode();
    /// object persistency
    //virtual bool SaveCmds(nPersistServer* ps);
    /// render geometry
    virtual bool RenderGeometry(nSceneServer* sceneServer, nRenderContext* renderContext);

    void SetUVStretch(int nr, float val);
    void SetDX7UVStretch(int nr, float val);
    void SetTexCount(int cnt);
    /// sets the part Index of this node
    void SetPartIndex(int partIndex);
    /// gets the part Index of this node
    int GetPartIndex() const;

protected:
    float uvStretch[6];
    float dx7uvStretch[6];
    int texCount;
    int partIndex;  // since multilayered nodes are splitted up, we need an index that identifies the part
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nMultiLayeredNode::SetUVStretch(int nr, float value)
{
    n_assert(nr >= 0 && nr < 6);
    this->uvStretch[nr] = value;
}
//------------------------------------------------------------------------------
/**
*/
inline
void
nMultiLayeredNode::SetDX7UVStretch(int nr, float value)
{
    n_assert(nr >= 0 && nr < 6);
    this->dx7uvStretch[nr] = value;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nMultiLayeredNode::SetTexCount(int cnt)
{
    this->texCount = cnt;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nMultiLayeredNode::SetPartIndex(int partIndex)
{
    this->partIndex = partIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMultiLayeredNode::GetPartIndex() const
{
    return this->partIndex;
}

#endif
