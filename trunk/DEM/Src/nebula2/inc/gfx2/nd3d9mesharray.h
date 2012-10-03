#ifndef N_D3D9MESHARRAY_H
#define N_D3D9MESHARRAY_H
//------------------------------------------------------------------------------
/**
    @class nD3D9MeshArray
    @ingroup Gfx2

    Holds an array of up to nGfxServer2::MaxVertexStreams meshes.
    Can be posted to the nD3D9GfxServer to assign all streams to the graphics
    device, with the correct vertex declaration based on the vertex components
    of the meshes.

    Because of limits in the D3D API the different streams must be different meshes.

    (C) 2004 RadonLabs GmbH
*/
#include "gfx2/nmesharray.h"
#include "gfx2/nd3d9mesh.h"

struct IDirect3DVertexBuffer9;
struct IDirect3DIndexBuffer9;
struct IDirect3DVertexDeclaration9;

//------------------------------------------------------------------------------
class nD3D9MeshArray : public nMeshArray
{
public:
    /// constructor
    nD3D9MeshArray();
    /// destructor
    virtual ~nD3D9MeshArray();
    /// get d3d vertex buffer for a mesh
    IDirect3DVertexBuffer9* GetVertexBuffer(int meshIndex) const;
    /// get d3d index buffer of first mesh
    IDirect3DIndexBuffer9* GetIndexBuffer() const;
    /// get the combined vertex declaration for the current meshes
    IDirect3DVertexDeclaration9* GetVertexDeclaration();

protected:
    /// override in subclass to perform actual resource loading
    virtual bool LoadResource();
    /// override in subclass to perform actual resource unloading
    virtual void UnloadResource();
    /// create the vertex declaration
    void CreateVertexDeclaration();

    IDirect3DVertexDeclaration9* vertexDeclaration;
};

//------------------------------------------------------------------------------
/**
    Get Vertex buffer of a mesh.
*/
inline
IDirect3DVertexBuffer9*
nD3D9MeshArray::GetVertexBuffer(int index) const
{
    nMesh2* mesh = this->GetMeshAt(index);
    if (0 != mesh)
    {
        return ((nD3D9Mesh*)mesh)->GetVertexBuffer();
    }
    return 0;
}

//------------------------------------------------------------------------------
/**
    Get Index buffer of the first mesh.
*/
inline
IDirect3DIndexBuffer9*
nD3D9MeshArray::GetIndexBuffer() const
{
    nMesh2* mesh = this->GetMeshAt(0);
    if (0 != mesh)
    {
        return ((nD3D9Mesh*)mesh)->GetIndexBuffer();
    }
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
inline
IDirect3DVertexDeclaration9*
nD3D9MeshArray::GetVertexDeclaration()
{
    n_assert(this->vertexDeclaration);
    return this->vertexDeclaration;
}

//------------------------------------------------------------------------------
#endif
