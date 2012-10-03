//------------------------------------------------------------------------------
//  nblendshaperenderer.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "deformers/nblendshaperenderer.h"
#include "deformers/nblendshapedeformer.h"
#include "gfx2/nmesharray.h"

//------------------------------------------------------------------------------
/**
*/
nBlendShapeRenderer::nBlendShapeRenderer() :
    initialized(false),
    useCpuBlending(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nBlendShapeRenderer::~nBlendShapeRenderer()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    This initializes the blend shape renderer either for shader-based or
    cpu-based blending.
*/
bool
nBlendShapeRenderer::Initialize(bool cpuBlending, nMeshArray* srcMeshArray)
{
    n_assert(!this->initialized);
    n_assert(srcMeshArray);

    this->initialized = true;
    this->useCpuBlending = cpuBlending;
    this->refSrcMeshArray = srcMeshArray;

    if (this->refDstMesh.isvalid())
    {
        this->refDstMesh->Release();
    }

    this->Setup();
    return true;
}

//------------------------------------------------------------------------------
/**
    Internal setup function. This is only needed when cpu blending
    is used.
*/
void
nBlendShapeRenderer::Setup()
{
    if (this->useCpuBlending)
    {
        // create a destination mesh as clone of the first mesh in the src mesh array
        nMesh2* srcMesh = this->refSrcMeshArray->GetMeshAt(0);

        n_assert(srcMesh->GetUsage() & nMesh2::ReadOnly);
        nString dstMeshName = srcMesh->GetName();
        dstMeshName.Append("_bsr");

        nMesh2* dstMesh = nGfxServer2::Instance()->NewMesh(dstMeshName);
        if (!dstMesh->IsLoaded())
        {
            // if the mesh hasn't been initialized yet, do it now
            int numVertices = srcMesh->GetNumVertices();
            int numIndices  = srcMesh->GetNumIndices();
            int numEdges    = srcMesh->GetNumEdges();

            dstMesh->SetUsage(nMesh2::ReadWrite);
            dstMesh->SetVertexComponents(srcMesh->GetVertexComponents());
            dstMesh->SetNumVertices(numVertices);
            dstMesh->SetNumIndices(numIndices);
            dstMesh->SetNumEdges(numEdges);
            dstMesh->Load();

            // copy over the index and edge data
            if (numIndices > 0)
            {
                void* srcIndices = srcMesh->LockIndices();
                void* dstIndices = dstMesh->LockIndices();
                memcpy(dstIndices, srcIndices, srcMesh->GetIndexBufferByteSize());
                dstMesh->UnlockIndices();
                srcMesh->UnlockIndices();
            }
            if (numEdges > 0)
            {
                void* srcEdges = srcMesh->LockEdges();
                void* dstEdges = dstMesh->LockEdges();
                memcpy(dstEdges, srcEdges, srcMesh->GetEdgeBufferByteSize());
                dstMesh->UnlockEdges();
                srcMesh->UnlockEdges();
            }
            dstMesh->SetState(nResource::Valid);
            this->refDstMesh = dstMesh;
        }
    }
}

//------------------------------------------------------------------------------
/**
    Toplevel render method, either performs cpu-based or vertex-shader
    based blending, based on the cpuBlending parameter to Initialize().
*/
void
nBlendShapeRenderer::Render(int meshGroupIndex, const nArray<float>& weights)
{
    n_assert(this->initialized);

    if (this->useCpuBlending)
    {
        if (!this->refDstMesh->IsValid())
        {
            this->Setup();
        }
        this->RenderCpuBlending(meshGroupIndex, weights);
    }
    else
    {
        this->RenderShaderBlending(meshGroupIndex, weights);
    }
}

//------------------------------------------------------------------------------
/**
    Perform rendering with CPU based blending...
*/
void
nBlendShapeRenderer::RenderCpuBlending(int meshGroupIndex, const nArray<float>& weights)
{
    // setup a blendshape deformer
    nBlendShapeDeformer blendShapeDeformer;
    //blendShapeDeformer.SetInputMeshArray();
}

//------------------------------------------------------------------------------
/**
    Perform rendering with GPU based blending...
*/
void
nBlendShapeRenderer::RenderShaderBlending(int meshGroupIndex, const nArray<float>& weights)
{
    // TODO: implementation
}
