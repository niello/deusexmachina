#ifndef N_SHAPENODE_H
#define N_SHAPENODE_H
//------------------------------------------------------------------------------
/**
    @class nShapeNode
    @ingroup SceneNodes

    @brief A shape node is the simplest actually visible object in the
    scene node class hierarchy.

    It is derived from nMaterialNode, and thus inherits transform and
    shader information. It adds a simple mesh which it can render.

    See also @ref N2ScriptInterface_nshapenode

    (C) 2002 RadonLabs GmbH
*/
#include "scene/nmaterialnode.h"
#include "gfx2/nmesh2.h"
#include "kernel/ndynautoref.h"

namespace Data
{
	class CBinaryReader;
}

//------------------------------------------------------------------------------
class nShapeNode : public nMaterialNode
{
public:
    /// constructor
    nShapeNode();

	virtual bool LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);

	/// load resources
    virtual bool LoadResources();
    /// unload resources
    virtual void UnloadResources();

    /// indicate to scene server that we offer geometry for rendering
    virtual bool HasGeometry() const;
    /// perform pre-instancing rendering of geometry
    virtual bool ApplyGeometry(nSceneServer* sceneServer);
    /// perform per-instance-rendering of geometry
    virtual bool RenderGeometry(nSceneServer* sceneServer, nRenderContext* renderContext);
    /// render debug information
    virtual void RenderDebug(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& modelMatrix);
    /// set the mesh usage flags required by this shape node
    void SetMeshUsage(int usage);
    /// get the mesh usage flags required by this shape node
    int GetMeshUsage() const;
    /// explicitly set the "needs vertex shader" mesh usage flag
    void SetNeedsVertexShader(bool b);
    /// explicitly get the "needs vertex shader" mesh usage flag
    bool GetNeedsVertexShader() const;

    /// set the mesh resource name
    void SetMesh(const nString& name);
    /// get the mesh resource name
    const nString& GetMesh() const;
    /// get mesh2 object
    nMesh2* GetMeshObject();

    /// set the mesh group index
    void SetGroupIndex(int i);
    /// get the mesh group index
    int GetGroupIndex() const;

protected:
    /// load mesh resource
    bool LoadMesh();
    /// unload mesh resource
    void UnloadMesh();

    int meshUsage;
    nRef<nMesh2> refMesh;
    nString meshName;
    int groupIndex;
};


//------------------------------------------------------------------------------
/**
    Indicate to scene server that we provide geometry
*/
inline
bool
nShapeNode::HasGeometry() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nShapeNode::SetGroupIndex(int i)
{
    this->groupIndex = i;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nShapeNode::GetGroupIndex() const
{
    return this->groupIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nShapeNode::SetMeshUsage(int usage)
{
    this->meshUsage = usage;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nShapeNode::GetMeshUsage() const
{
    return this->meshUsage;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nShapeNode::SetNeedsVertexShader(bool b)
{
    if (b)
    {
        this->meshUsage |= nMesh2::NeedsVertexShader;
    }
    else
    {
        this->meshUsage &= ~nMesh2::NeedsVertexShader;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nShapeNode::GetNeedsVertexShader() const
{
    return 0 != (this->meshUsage & nMesh2::NeedsVertexShader);
}

//------------------------------------------------------------------------------
#endif

