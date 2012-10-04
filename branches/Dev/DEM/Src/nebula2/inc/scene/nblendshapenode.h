#ifndef N_BLENDSHAPENODE_H
#define N_BLENDSHAPENODE_H
//------------------------------------------------------------------------------
/**
    @class nBlendShapeNode
    @ingroup Scene

    See also @ref N2ScriptInterface_nblendshapenode

    (C) 2004 RadonLabs GmbH
*/
#include "scene/nmaterialnode.h"
#include "gfx2/ngfxserver2.h"
#include "gfx2/nmesharray.h"

//------------------------------------------------------------------------------
class nBlendShapeNode : public nMaterialNode
{
public:
    /// constructor
    nBlendShapeNode();
    /// destructor
    virtual ~nBlendShapeNode();
    /// object persistency
    //virtual bool SaveCmds(nPersistServer* ps);
    /// load resources
    virtual bool LoadResources();
    /// unload resources
    virtual void UnloadResources();

    /// indicate to scene server that we offer geometry for rendering
    virtual bool HasGeometry() const;
    /// get the mesh usage flags required by this shape node
    virtual int GetMeshUsage() const;
    /// perform pre-instancing rendering of geometry
    virtual bool ApplyGeometry(nSceneServer* sceneServer);
    /// perform per-instance-rendering of geometry
    virtual bool RenderGeometry(nSceneServer* sceneServer, nRenderContext* renderContext);

    /// set the mesh resource name for the specified index
    void SetMeshAt(int index, const char* name);
    /// get the mesh resource name for the specified index
    const char* GetMeshAt(int index) const;
    /// set the mesh group index
    void SetGroupIndex(int i);
    /// get the mesh group index
    int GetGroupIndex() const;
    /// set the local bounding box for the specified shape
    void SetLocalBoxAt(int index, const bbox3& localBox);
    /// get the local bounding box for the specified shape
    const bbox3& GetLocalBoxAt(int index) const;
    /// set the weight of the specified shape
    void SetWeightAt(int index, float weight);
    /// get the weight of the specified shape
    float GetWeightAt(int index) const;
    /// get number of valid shapes
    int GetNumShapes() const;

    /// max number of shapes
    enum
    {
        MaxShapes = 7,
    };

protected:
    /// update shader state with new weights
    void UpdateShaderState();

    class Shape
    {
    public:
        /// constructor
        Shape();
        /// clear object
        void Clear();

        nString meshName;
        bbox3 localBox;
        float weight;
    };

    int numShapes;
    int groupIndex;
    nFixedArray<Shape> shapeArray;
    nRef<nMeshArray> refMeshArray;
};

//------------------------------------------------------------------------------
/**
*/
inline
nBlendShapeNode::Shape::Shape() :
    weight(0.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nBlendShapeNode::Shape::Clear()
{
    this->localBox = bbox3();
    this->meshName = 0;
    this->weight = 0.0f;
}

//------------------------------------------------------------------------------
/**
    return the number of valid shapes
*/
inline
int
nBlendShapeNode::GetNumShapes() const
{
    return this->numShapes;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nBlendShapeNode::GetGroupIndex() const
{
    return this->groupIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nBlendShapeNode::SetGroupIndex(int i)
{
    if (i != this->groupIndex)
    {
        this->groupIndex = i;
        this->resourcesValid = false;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
const char*
nBlendShapeNode::GetMeshAt(int index) const
{
    return this->shapeArray[index].meshName.Get();
}

//------------------------------------------------------------------------------
/**
    Set the local box for the specified shape.
*/
inline
void
nBlendShapeNode::SetLocalBoxAt(int index, const bbox3& localBox)
{
    this->shapeArray[index].localBox = localBox;
}

//------------------------------------------------------------------------------
/**
*/
inline
const bbox3&
nBlendShapeNode::GetLocalBoxAt(int index) const
{
    return this->shapeArray[index].localBox;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nBlendShapeNode::SetWeightAt(int index, float weight)
{
    this->shapeArray[index].weight = weight;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nBlendShapeNode::GetWeightAt(int index) const
{
    return this->shapeArray[index].weight;
}

//------------------------------------------------------------------------------
#endif

