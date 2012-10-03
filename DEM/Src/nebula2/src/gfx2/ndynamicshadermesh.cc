//------------------------------------------------------------------------------
//  ndynamicshadermesh.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "gfx2/ndynamicshadermesh.h"

//------------------------------------------------------------------------------
/**
*/
nDynamicShaderMesh::nDynamicShaderMesh()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nDynamicShaderMesh::~nDynamicShaderMesh()
{
    if (this->refShader.isvalid())
    {
        this->refShader->Release();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
nDynamicShaderMesh::SetShader(nShader2* shader)
{
    n_assert(shader);
    this->refShader = shader;
}

//------------------------------------------------------------------------------
/**
*/
nShader2*
nDynamicShaderMesh::GetShader() const
{
    return this->refShader.isvalid() ? this->refShader.get() : 0;
}

//------------------------------------------------------------------------------
/**
    Return true if object is valid, otherwise Initialize() and SetShader() must be called
    to prepare the object for rendering. A dynamic mesh can become
    invalid anytime, thus make sure you check before each call to
    Begin()/End().
*/
bool
nDynamicShaderMesh::IsValid() const
{
    return this->refShader.isvalid() && this->refShader->IsValid() && nDynamicMesh::IsValid();
}

//------------------------------------------------------------------------------
/**
    Begin non-indexed rendering to the dynamic mesh.

    @param  vertexPointer   [out] will be filled with a pointer to the vertex buffer
    @param  maxNumVertices  [out] max number of vertices before calling Swap() or End()
*/
void
nDynamicShaderMesh::Begin(float*& vertexPointer, int& maxNumVertices)
{
    n_assert(!this->indexedRendering);

    nMesh2* mesh = this->refMesh.get();
    vertexPointer  = mesh->LockVertices();
    maxNumVertices = mesh->GetNumVertices();
}

//------------------------------------------------------------------------------
/**
    Do an intermediate swap for non-indexed rendering.

    @param  numVertices     [in] number of vertices written to the vertex buffer
    @param  vertexPointer   [out] new vertex buffer pointer for writing new vertices
*/
void
nDynamicShaderMesh::Swap(int numVertices, float*& vertexPointer)
{
    n_assert(!this->indexedRendering);

    nMesh2* mesh = this->refMesh.get();
    nShader2* shader = this->refShader.get();

    mesh->UnlockVertices();
    nGfxServer2::Instance()->SetMesh(mesh, mesh);
    nGfxServer2::Instance()->SetVertexRange(0, numVertices);
    nGfxServer2::Instance()->SetShader(shader);
    int numPass = shader->Begin(true);
    int curPass;
    for (curPass = 0; curPass < numPass; curPass++)
    {
        shader->BeginPass(curPass);
        nGfxServer2::Instance()->DrawNS(this->primitiveType);
        shader->EndPass();
    }
    shader->End();
    vertexPointer = mesh->LockVertices();
}

//------------------------------------------------------------------------------
/**
    Finish non-indexed rendering.

    @param  numVertices     number of valid vertices in the vertex buffer
*/
void
nDynamicShaderMesh::End(int numVertices)
{
    n_assert(!this->indexedRendering);

    nMesh2* mesh = this->refMesh.get();
    nShader2* shader = this->refShader.get();

    mesh->UnlockVertices();
    if (numVertices > 0)
    {
        nGfxServer2::Instance()->SetMesh(mesh, mesh);
        nGfxServer2::Instance()->SetVertexRange(0, numVertices);
        nGfxServer2::Instance()->SetShader(shader);

        int numPass = shader->Begin(true);
        int curPass;
        for (curPass = 0; curPass < numPass; curPass++)
        {
            shader->BeginPass(curPass);
            nGfxServer2::Instance()->DrawNS(this->primitiveType);
            shader->EndPass();
        }
        shader->End();
    }
}
