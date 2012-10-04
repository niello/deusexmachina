#ifndef N_D3D9MESH_H
#define N_D3D9MESH_H
//------------------------------------------------------------------------------
/**
    @class nD3D9Mesh
    @ingroup Gfx2

    nMesh2 subclass for Direct3D9.

    (C) 2003 RadonLabs GmbH
*/
#include "gfx2/nmesh2.h"
#include "gfx2/nd3d9server.h"

//------------------------------------------------------------------------------
class nD3D9Mesh : public nMesh2
{
public:
    /// constructor
    nD3D9Mesh();
    /// destructor
	virtual ~nD3D9Mesh() { if (IsLoaded()) Unload(); }

	virtual bool CreateNew(DWORD VtxCount, DWORD IdxCount, int Usage, int VtxComponents);

	virtual bool CanLoadAsync() const { return true; }
    /// lock vertex buffer
    virtual float* LockVertices();
    /// unlock vertex buffer
    virtual void UnlockVertices();
    /// lock index buffer
    virtual ushort* LockIndices();
    /// unlock index buffer
    virtual void UnlockIndices();
    /// get an estimated byte size of the resource data (for memory statistics)
    virtual int GetByteSize();

    /// optimize the mesh (can be redefined for each platform)
    virtual bool OptimizeMesh(OptimizationFlag flags, float * vertices, int numVertices, ushort * indices, int numIndices);

protected:
    /// load mesh resource
    virtual bool LoadResource();
    /// unload mesh resource
    virtual void UnloadResource();
    /// called when contained resource may become lost
    virtual void OnLost();
    /// called when contained resource may be restored
    virtual void OnRestored();
    /// create the d3d vertex buffer
    virtual void CreateVertexBuffer();
    /// create the d3d index buffer
    virtual void CreateIndexBuffer();

private:
    friend class nD3D9Server;
    friend class nD3D9MeshArray;

    /// create the vertex declaration
    void CreateVertexDeclaration();
    /// get d3d vertex buffer
    IDirect3DVertexBuffer9* GetVertexBuffer();
    /// get d3d index buffer
    IDirect3DIndexBuffer9* GetIndexBuffer();
    /// get the d3d vertex declaration
    IDirect3DVertexDeclaration9* GetVertexDeclaration();

    /// optimize by reorganizing faces
    void OptimizeFaces(ushort* indices, int numFaces, int numVertices);
    /// optimize by reorganizing vertices
    void OptimizeVertices(float* vertices, ushort* indices, int numVertices, int numFaces);

    DWORD d3dVBLockFlags;
    DWORD d3dIBLockFlags;
    IDirect3DVertexBuffer9* vertexBuffer;
    IDirect3DIndexBuffer9* indexBuffer;
    IDirect3DVertexDeclaration9* vertexDeclaration;
    void* privVertexBuffer;                     // valid if Usage==ReadOnly
    void* privIndexBuffer;                      // valid if Usage==ReadOnly
};

//------------------------------------------------------------------------------
/**
*/
inline
IDirect3DVertexBuffer9*
nD3D9Mesh::GetVertexBuffer()
{
    n_assert(this->IsLoaded());
    return this->vertexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
inline
IDirect3DIndexBuffer9*
nD3D9Mesh::GetIndexBuffer()
{
    n_assert(this->IsLoaded());
    return this->indexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
inline
IDirect3DVertexDeclaration9*
nD3D9Mesh::GetVertexDeclaration()
{
    n_assert(this->IsLoaded());
    return this->vertexDeclaration;
}
//------------------------------------------------------------------------------
#endif


