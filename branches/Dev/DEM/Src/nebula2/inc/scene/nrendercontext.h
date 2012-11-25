#ifndef N_RENDERCONTEXT_H
#define N_RENDERCONTEXT_H
//------------------------------------------------------------------------------
/**
    @class nRenderContext
    @ingroup Scene

    @brief A nRenderContext object holds frame persistent data for nSceneNode
    hierarchies and serves as the central communication point between
    the client app and nSceneNode hierarchies.

    (C) 2002 RadonLabs GmbH
*/
#include "kernel/nref.h"
#include "variable/nvariablecontext.h"
#include "gfx2/nshaderparams.h"
#include "mathlib/bbox.h"

class nSceneNode;

//------------------------------------------------------------------------------
class nRenderContext : public nVariableContext
{
public:
    /// flags
    enum Flag
    {
        ShapeVisible = (1<<0),
        ShadowVisible = (1<<1),
        Occluded = (1<<2),
        CastShadows = (1<<3),
        DoOcclusionQuery = (1<<4),
    };
    /// constructor
    nRenderContext();
    /// destructor
    ~nRenderContext();
    /// set the current frame id
    void SetFrameId(uint id);
    /// get the current frame id
    uint GetFrameId() const;
    /// set the current transformation
    void SetTransform(const matrix44& m);
    /// get the current transformation
    const matrix44& GetTransform() const;
    /// set the render context's root scene node
    void SetRootNode(nSceneNode* node);
    /// get the render context's root scene node
    nSceneNode* GetRootNode() const;
    /// set global bounding box
    void SetGlobalBox(const bbox3& b);
    /// get global bounding box
    const bbox3& GetGlobalBox() const;
    /// return true if valid (if root node is set)
    bool IsValid() const;
    /// reset the link array
    void ClearLinks();
    /// add a link to another render context
    void AddLink(nRenderContext* link);
    /// get the number of links
    int GetNumLinks() const;
    /// get linked render context at index
    nRenderContext* GetLinkAt(int index) const;
    /// access to shader parameter overrides
    nShaderParams& GetShaderOverrides();
    /// appends a new var to localVarArray, returns the index
    int AddLocalVar(const nVariable& value);
    /// returns local variable at given index
    nVariable& GetLocalVar(int index);
    /// find local variable by variable handle
    nVariable* FindLocalVar(nVariable::Handle handle);
    /// set per-context shadow intensity value
    void SetShadowIntensity(float i);
    /// get per-context shadow intensity value
    float GetShadowIntensity() const;
    /// set flags
    void SetFlag(Flag f, bool b);
    /// get visibility hint
    bool GetFlag(Flag f) const;

private:
    friend class nSceneServer;

    /// set scene server group index, this is read/written by the scene server
    void SetSceneGroupIndex(int i);
    /// get scene server group index, this is read/written by the scene server
    int GetSceneGroupIndex() const;
    /// set scene server light info index
    void SetSceneLightIndex(int i);
    /// get scene server light info index
    int GetSceneLightIndex() const;

    uint frameId;
    uint flags;
    int sceneGroupIndex;
    int sceneLightIndex;
    matrix44 transform;
    bbox3 globalBox;
    nRef<nSceneNode> rootNode;
    float priority;
    float shadowIntensity;
    nArray<nRenderContext*> linkArray;
    nArray<nVariable> localVarArray;
    nShaderParams shaderOverrides;
};

//------------------------------------------------------------------------------
/**
*/
inline
nRenderContext::nRenderContext() :
    frameId(0xffffffff),
    flags(ShapeVisible | ShadowVisible | CastShadows | DoOcclusionQuery),
    sceneGroupIndex(-1),
    sceneLightIndex(-1),
    priority(1.0f),
    shadowIntensity(1.0f)
{
    this->globalBox.set(vector3(0.0f, 0.0f, 0.0f), vector3(1000.0f, 1000.0f, 1000.0f));
    this->linkArray.SetFlags(nArray<nRenderContext*>::DoubleGrowSize);
}

//------------------------------------------------------------------------------
/**
*/
inline
nRenderContext::~nRenderContext()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRenderContext::SetFrameId(uint id)
{
    this->frameId = id;
}

//------------------------------------------------------------------------------
/**
*/
inline
uint
nRenderContext::GetFrameId() const
{
    return this->frameId;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRenderContext::SetTransform(const matrix44& m)
{
    this->transform = m;
}

//------------------------------------------------------------------------------
/**
*/
inline
const matrix44&
nRenderContext::GetTransform() const
{
    return this->transform;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRenderContext::SetRootNode(nSceneNode* node)
{
    n_assert(node);
    this->rootNode = node;
}

//------------------------------------------------------------------------------
/**
*/
inline
nSceneNode*
nRenderContext::GetRootNode() const
{
    return this->rootNode.get();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRenderContext::ClearLinks()
{
    this->linkArray.Clear();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRenderContext::AddLink(nRenderContext* link)
{
    n_assert(link);
    this->linkArray.Append(link);
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nRenderContext::GetNumLinks() const
{
    return this->linkArray.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline
nRenderContext*
nRenderContext::GetLinkAt(int index) const
{
    return this->linkArray[index];
}

//------------------------------------------------------------------------------
/**
*/
inline
nShaderParams&
nRenderContext::GetShaderOverrides()
{
    return this->shaderOverrides;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nRenderContext::AddLocalVar(const nVariable& value)
{
    this->localVarArray.Append(value);
    return this->localVarArray.Size()-1;
}

//------------------------------------------------------------------------------
/**
*/
inline
nVariable&
nRenderContext::GetLocalVar(int index)
{
    return this->localVarArray.At(index);
}

//------------------------------------------------------------------------------
/**
*/
inline
nVariable*
nRenderContext::FindLocalVar(nVariable::Handle handle)
{
    int i;
    int num = this->localVarArray.Size();
    for (i = 0; i < num; i++)
    {
        if (this->localVarArray[i].GetHandle() == handle)
        {
            return &(this->localVarArray[i]);
        }
    }
    // fallthrough: not found
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nRenderContext::IsValid() const
{
    return this->rootNode.isvalid();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRenderContext::SetSceneGroupIndex(int i)
{
    this->sceneGroupIndex = i;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nRenderContext::GetSceneGroupIndex() const
{
    return this->sceneGroupIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRenderContext::SetSceneLightIndex(int i)
{
    this->sceneLightIndex = i;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nRenderContext::GetSceneLightIndex() const
{
    return this->sceneLightIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRenderContext::SetShadowIntensity(float i)
{
    this->shadowIntensity = i;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nRenderContext::GetShadowIntensity() const
{
    return this->shadowIntensity;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRenderContext::SetFlag(Flag f, bool b)
{
    if (b) this->flags |= f;
    else   this->flags &= ~f;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nRenderContext::GetFlag(Flag f) const
{
    return 0 != (this->flags & f);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRenderContext::SetGlobalBox(const bbox3& b)
{
    this->globalBox = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
const bbox3&
nRenderContext::GetGlobalBox() const
{
    return this->globalBox;
}

//------------------------------------------------------------------------------
#endif
