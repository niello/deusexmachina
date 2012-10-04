//------------------------------------------------------------------------------
//  nblendshapenode_main.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "scene/nblendshapenode.h"
#include "gfx2/nmesh2.h"
#include "scene/nanimator.h"

nNebulaClass(nBlendShapeNode, "nmaterialnode");

//------------------------------------------------------------------------------
/**
*/
nBlendShapeNode::nBlendShapeNode() :
    numShapes(0),
    groupIndex(0),
    shapeArray(MaxShapes)
{
    //empty
}

//------------------------------------------------------------------------------
/**
*/
nBlendShapeNode::~nBlendShapeNode()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Indicate to scene server that we provide geometry
*/
bool
nBlendShapeNode::HasGeometry() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
    This method must return the mesh usage flag combination required by
    this shape node class. Subclasses should override this method
    based on their requirements.

    @return     a combination on nMesh2::Usage flags
*/
int
nBlendShapeNode::GetMeshUsage() const
{
    return nMesh2::WriteOnce | nMesh2::NeedsVertexShader;
}

//------------------------------------------------------------------------------
/**
    Load the resources needed by this object.
*/
bool
nBlendShapeNode::LoadResources()
{
    nMaterialNode::LoadResources();
    if (!this->refMeshArray.isvalid())
    {
        this->refMeshArray = nGfxServer2::Instance()->NewMeshArray(0);
    }

    // update resouce filenames in mesharray
    int i;
    for (i = 0; i < this->GetNumShapes(); i++)
    {
        this->refMeshArray->SetFilenameAt(i, this->shapeArray[i].meshName);
        this->refMeshArray->SetUsageAt(i, this->GetMeshUsage());
    }
    this->resourcesValid &= this->refMeshArray->Load();

    // update shape bounding boxes
    if (true == this->resourcesValid)
    {
        for (i = 0; i < this->GetNumShapes(); i++)
        {
            nMesh2* mesh = this->refMeshArray->GetMeshAt(i);
            if (0 != mesh)
            {
                this->shapeArray[i].localBox = mesh->Group(this->groupIndex).Box;
            }
        }
    }
    return this->resourcesValid;
}

//------------------------------------------------------------------------------
/**
    Unload the resources.
*/
void
nBlendShapeNode::UnloadResources()
{
    nMaterialNode::UnloadResources();
    if (this->refMeshArray.isvalid())
    {
        this->refMeshArray->Unload();
        this->refMeshArray->Release();
        this->refMeshArray.invalidate();
    }
}

//------------------------------------------------------------------------------
/**
    Set the mesh resource name at index.
    Updates the number of current valid shapes.

    @param  index
    @param  name    name of the resource to set, 0 to unset a resource
*/
void
nBlendShapeNode::SetMeshAt(int index, const char* name)
{
    n_assert((index >= 0) && (index < MaxShapes));
    if (this->shapeArray[index].meshName != name)
    {
        this->resourcesValid = false;
        this->shapeArray[index].meshName = name;

        if (0 != name)
        {
            // increase shapes count
            this->numShapes = n_max(index+1, this->numShapes);
        }
        else
        {
            // decrease shapes count if this was the last element
            if (index + 1 == this->numShapes)
            {
                this->numShapes--;
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
    Gives the weights to the shader
*/
void
nBlendShapeNode::UpdateShaderState()
{
    // set shader parameter
    nShader2* shader = nGfxServer2::Instance()->GetShader();
    n_assert(shader);

    int numShapes = this->GetNumShapes();
    shader->SetInt(nShaderState::VertexStreams, numShapes);
    if (numShapes > 0)
    {
        nFloat4 weights = {0.0f, 0.0f, 0.0f, 0.0f};
        if (numShapes > 0) weights.x = this->GetWeightAt(0);
        if (numShapes > 1) weights.y = this->GetWeightAt(1);
        if (numShapes > 2) weights.z = this->GetWeightAt(2);
        if (numShapes > 3) weights.w = this->GetWeightAt(3);
        shader->SetFloat4(nShaderState::VertexWeights1, weights);
    }
    if (numShapes > 4)
    {
        nFloat4 weights = {0.0f, 0.0f, 0.0f, 0.0f};
        if (numShapes > 4) weights.x = this->GetWeightAt(4);
        if (numShapes > 5) weights.y = this->GetWeightAt(5);
        if (numShapes > 6) weights.z = this->GetWeightAt(6);
        if (numShapes > 7) weights.w = this->GetWeightAt(7);
        shader->SetFloat4(nShaderState::VertexWeights2, weights);
    }
}

//------------------------------------------------------------------------------
/**
    Perform pre-instancing actions needed for rendering geometry. This
    is called once before multiple instances of this shape node are
    actually rendered.
*/
bool
nBlendShapeNode::ApplyGeometry(nSceneServer* /*sceneServer*/)
{
    // set mesh, vertex and index range
    nGfxServer2::Instance()->SetMeshArray(this->refMeshArray.get());
    const nMeshGroup& curGroup = this->refMeshArray->GetMeshAt(0)->Group(this->groupIndex);
    nGfxServer2::Instance()->SetVertexRange(curGroup.FirstVertex, curGroup.NumVertices);
    nGfxServer2::Instance()->SetIndexRange(curGroup.FirstIndex, curGroup.NumIndices);
    return true;
}

//------------------------------------------------------------------------------
/**
    Update geometry, set as current mesh in the gfx server and
    call nGfxServer2::DrawIndexed().

    - 15-Jan-04 floh    AreResourcesValid()/LoadResource() moved to scene server
    - 01-Feb-05 floh    use nBlendShapeDeformer on CPU
*/
bool
nBlendShapeNode::RenderGeometry(nSceneServer* /*sceneServer*/, nRenderContext* renderContext)
{
    // invoke blend shape animators (manipulating the weights)
    this->InvokeAnimators(nAnimator::BlendShape, renderContext);

    // update shader state
    this->UpdateShaderState();

    // draw the geometry
    nGfxServer2::Instance()->DrawIndexedNS(nGfxServer2::TriangleList);
    return true;
}
