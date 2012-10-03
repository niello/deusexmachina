//------------------------------------------------------------------------------
//  ndynamicmesh.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "gfx2/ndynamicmesh.h"

//------------------------------------------------------------------------------
/**
*/
nDynamicMesh::nDynamicMesh() :
    indexedRendering(true),
    primitiveType(nGfxServer2::TriangleList)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nDynamicMesh::~nDynamicMesh()
{
    if (this->refMesh.isvalid())
    {
        this->refMesh->Release();
    }
}

//------------------------------------------------------------------------------
/**
    Return true if object is valid, otherwise Initialize() must be called
    to prepare the object for rendering. A dynamic mesh can become
    invalid anytime, thus make sure you check before each call to
    Begin()/End().
*/
bool
nDynamicMesh::IsValid() const
{
    if (this->refMesh.isvalid())
    {
        if (!this->refMesh->IsLoaded())
        {
            return false;
        }
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    Initialize the dynamic mesh. This will create or lookup a mesh
    which is shared with all other dynamic mesh objects with the
    same vertex components and usage flags.
    This method must be called whenever a call to IsValid() returns false.

    @param  primType            primitive type
    @param  vertexComponents    vertex component mask (see nMesh2)
    @param  usageFlags          usage flags (see nMesh2)
    @param  indexed             true if indexed primitive rendering is intended
    @param  rsrcBaseName        
    @param  shared              true if it should use a resource name based on the vertex component mask and usage flags
    @return                     true if initialized successful
*/
bool
nDynamicMesh::Initialize(nGfxServer2::PrimitiveType primType,
                         int vertexComponents,
                         int usageFlags,
                         bool indexed,
                         const nString& rsrcBaseName,
                         bool shared)
{
    this->primitiveType = primType;
    this->indexedRendering = indexed;

    nMesh2* mesh = 0;
    if (!this->refMesh.isvalid())
    {
        // build resource sharing name
        if (shared)
        {
            n_assert(rsrcBaseName.Length() < 64);
            char resName[128];
            strcpy(resName, rsrcBaseName.Get());
            int charIndex = (int)strlen(resName);
            if (vertexComponents & nMesh2::Coord)       resName[charIndex++] = 'a';
            if (vertexComponents & nMesh2::Normal)      resName[charIndex++] = 'b';
            if (vertexComponents & nMesh2::Tangent)     resName[charIndex++] = 'c';
            if (vertexComponents & nMesh2::Binormal)    resName[charIndex++] = 'd';
            if (vertexComponents & nMesh2::Color)       resName[charIndex++] = 'e';
            if (vertexComponents & nMesh2::Uv0)         resName[charIndex++] = 'f';
            if (vertexComponents & nMesh2::Uv1)         resName[charIndex++] = 'g';
            if (vertexComponents & nMesh2::Uv2)         resName[charIndex++] = 'h';
            if (vertexComponents & nMesh2::Uv3)         resName[charIndex++] = 'i';
            if (vertexComponents & nMesh2::Weights)     resName[charIndex++] = 'f';
            if (vertexComponents & nMesh2::JIndices)    resName[charIndex++] = 'g';
            if (usageFlags & nMesh2::PointSprite)       resName[charIndex++] = 'j';
            if (this->indexedRendering)                 resName[charIndex++] = 'l';
            resName[charIndex] = 0;
            // create shared mesh object
            mesh = nGfxServer2::Instance()->NewMesh(resName);
        }
        else
        {
            mesh = nGfxServer2::Instance()->NewMesh(0);
        }

        n_assert(mesh);
        this->refMesh = mesh;
    }
    else
    {
        mesh = this->refMesh;
    }

    // initialize the mesh
    if (!mesh->IsLoaded())
    {
        mesh->SetUsage(usageFlags | nMesh2::WriteOnly);
        mesh->SetVertexComponents(vertexComponents);
        mesh->SetNumVertices(VertexBufferSize);
        if (indexed)
        {
            mesh->SetNumIndices(IndexBufferSize);
        }
        else
        {
            mesh->SetNumIndices(0);
        }
        mesh->Load();
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    Begin indexed rendering to the dynamic mesh. The method returns pointers
    to the beginning of the vertex buffer and index buffer, and the
    maximum number of vertices and indices which can be written
    before Swap() or End() must be called.

    @param  vertexPointer   [out] will be filled with a pointer to the vertex buffer
    @param  indexPointer    [out] will be filled with a pointer to the index buffer
    @param  maxNumVertices  [out] max number of vertices before calling Swap() or End()
    @param  maxNumIndices   [out] max number of indices before calling Swap() or End()
*/
void
nDynamicMesh::BeginIndexed(float*& vertexPointer,
                           ushort*& indexPointer,
                           int& maxNumVertices,
                           int& maxNumIndices)
{
    n_assert(this->indexedRendering);
    nMesh2* mesh = this->refMesh.get();

    nGfxServer2::Instance()->SetMesh(mesh, mesh);

    vertexPointer  = mesh->LockVertices();
    indexPointer   = mesh->LockIndices();
    maxNumVertices = mesh->GetNumVertices();
    maxNumIndices  = mesh->GetNumIndices();
}

//------------------------------------------------------------------------------
/**
    Do an intermediate swap. Call this method when the max number of
    vertices or the max number of indices returned by Begin() have
    been written into the vertex and index buffers. The internal
    dynamic mesh will be rendered, and render attributes will be returned.
    Note that the contents of the vertex and index buffer will be discarded,
    so everything must be overwritten!

    This method will unlock the global dynamic mesh, immediately render it
    through the gfx server, and lock it again.

    @param  numVertices     [in] number of vertices written to the vertex buffer
    @param  numIndices      [in] number of indices written to the index buffer
    @param  vertexPointer   [out] new vertex buffer pointer for writing new vertices
    @param  indexPointer    [out] new index buffer pointer for writing new indices
*/
void
nDynamicMesh::SwapIndexed(int numVertices, int numIndices, float*& vertexPointer, ushort*& indexPointer)
{
    n_assert(this->indexedRendering);

    nMesh2* mesh = this->refMesh.get();
    mesh->UnlockVertices();
    mesh->UnlockIndices();
    nGfxServer2::Instance()->SetVertexRange(0, numVertices);
    nGfxServer2::Instance()->SetIndexRange(0, numIndices);
    nGfxServer2::Instance()->DrawIndexedNS(this->primitiveType);
    vertexPointer = mesh->LockVertices();
    indexPointer  = mesh->LockIndices();
}

//------------------------------------------------------------------------------
/**
    Finish indexed rendering. Call this method when no more dynamic geometry
    needs to be rendered. This method will do a final DrawIndexed() call to
    the gfx server with the remaining valid vertices and indices.

    @param  numVertices     number of valid vertices in the vertex buffer
    @param  numIndices      number of valid indices in the vertex buffer
*/
void
nDynamicMesh::EndIndexed(int numVertices, int numIndices)
{
    n_assert(this->indexedRendering);
    nMesh2* mesh = this->refMesh.get();

    mesh->UnlockVertices();
    mesh->UnlockIndices();
    nGfxServer2::Instance()->SetVertexRange(0, numVertices);
    nGfxServer2::Instance()->SetIndexRange(0, numIndices);
    nGfxServer2::Instance()->DrawIndexedNS(this->primitiveType);
    nGfxServer2::Instance()->SetMesh(0, 0);
}

//------------------------------------------------------------------------------
/**
    Begin non-indexed rendering to the dynamic mesh.

    @param  vertexPointer   [out] will be filled with a pointer to the vertex buffer
    @param  maxNumVertices  [out] max number of vertices before calling Swap() or End()
*/
void
nDynamicMesh::Begin(float*& vertexPointer, int& maxNumVertices)
{
    n_assert(!this->indexedRendering);

    nMesh2* mesh = this->refMesh.get();
    nGfxServer2::Instance()->SetMesh(mesh, mesh);

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
nDynamicMesh::Swap(int numVertices, float*& vertexPointer)
{
    n_assert(!this->indexedRendering);
    nMesh2* mesh = this->refMesh.get();

    mesh->UnlockVertices();
    nGfxServer2::Instance()->SetVertexRange(0, numVertices);
    nGfxServer2::Instance()->DrawNS(this->primitiveType);
    vertexPointer = mesh->LockVertices();
}

//------------------------------------------------------------------------------
/**
    Finish non-indexed rendering.

    @param  numVertices     number of valid vertices in the vertex buffer
*/
void
nDynamicMesh::End(int numVertices)
{
    n_assert(!this->indexedRendering);

    this->refMesh->UnlockVertices();
    if (0 != numVertices)
    {
        nGfxServer2::Instance()->SetVertexRange(0, numVertices);
        nGfxServer2::Instance()->DrawNS(this->primitiveType);
    }
    nGfxServer2::Instance()->SetMesh(0, 0);
}
