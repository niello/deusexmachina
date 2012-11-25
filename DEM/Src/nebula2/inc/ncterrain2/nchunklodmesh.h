#ifndef N_CHUNKLODMESH_H
#define N_CHUNKLODMESH_H
//------------------------------------------------------------------------------
/**
    @class nChunkLodMesh
    @ingroup NCTerrain2

    @brief A single mesh chunk in a ChunkLOD vertex tree. This is basically
    a port of the original nChunkData class to Nebula2's resource subsystem.

    A nChunkLodMesh object takes the filename of a <tt>.chu</tt> file, and a
    position and byte size inside the file which identifies a mesh chunk and
    creates a Nebula2 nMesh2 object from it.

    Based on Thatcher Ulrich's original ChunkLOD implementation, and Magnus
    Anderholm's and Gary Haussmann's Nebula implementation of ChunkLOD.
    
    Copyright (c) 2002 Magnus Anderholm
    Modified 2003 Gary Haussmann
    (C) 2003 RadonLabs GmbH
*/
#include "resource/nresource.h"
#include "gfx2/ngfxserver2.h"
#include "gfx2/nmesh2.h"
#include "gfx2/nshader2.h"
#include "ncterrain2/nchunklodrenderparams.h"

//------------------------------------------------------------------------------
class nChunkLodMesh : public nResource
{
public:
    /// constructor
    nChunkLodMesh();
    /// destructor
    virtual ~nChunkLodMesh();
    /// subclasses must indicate to nResource whether async mode is supported
    virtual bool CanLoadAsync() const;
    /// get the resulting mesh resource object (only after IsValid() returns true!)
    nMesh2* GetMesh();
    /// set alternative shared file object pointing to open .chu file
	void SetFile(Data::CFileStream* file);
    /// get alternative shared file object pointing to .chu file
    Data::CFileStream* GetFile() const;
    /// set position and size inside .chu file
    void SetFileLocation(int pos);
    /// get position and size inside .chu file
    int GetFileLocation();
    /// set decompression parameters (sx, sy, sz)
    void SetVertexScale(const vector3& s);
    /// get decompression parameters 
    const vector3& GetVertexScale() const;
    /// set vertex offset
    void SetVertexOffset(const vector3& offset);
    /// get vertex offset
    const vector3& GetVertexOffset() const;
    /// render the mesh, return number of primitives
    int Render(const nChunkLodRenderParams& renderParams);

protected:
    /// load part of .chu file into internal nMesh2 object
    virtual bool LoadResource();
    /// unload internal nMesh2 object
    virtual void UnloadResource();

    int fileChunkPos;           // position of chunk in file
    vector3 vertexScale;        // vertex decompression parameters
    vector3 vertexOffset;       // world space offset for vertex decompression
    nFloat4 matDiffuse;          // a material diffuse color for debugging

    Data::CFileStream* chuFile;
    bool extChuFile;            // true if file object externally provided 

    nRef<nMesh2> refMesh;       // the mesh object
};

//------------------------------------------------------------------------------
/**
    Render the mesh.
*/
inline
int
nChunkLodMesh::Render(const nChunkLodRenderParams& renderParams)
{
    // set random material color for debugging
    if (renderParams.shader->IsParameterUsed(nShaderState::MatDiffuse))
        renderParams.shader->SetFloat4(nShaderState::MatDiffuse, this->matDiffuse);

	nMesh2* mesh = this->refMesh.get();
    n_assert(mesh->IsValid());
    renderParams.gfxServer->SetMesh(mesh, mesh);
    renderParams.gfxServer->SetVertexRange(0, mesh->GetNumVertices());
    renderParams.gfxServer->SetIndexRange(0, mesh->GetNumIndices());
    renderParams.gfxServer->DrawIndexedNS(nGfxServer2::TriangleStrip);
    return mesh->GetNumIndices() - 2;
}

//------------------------------------------------------------------------------
/**
    Get pointer to internal mesh object.
*/
inline
nMesh2*
nChunkLodMesh::GetMesh()
{
    return this->refMesh.get();
}

//------------------------------------------------------------------------------
/**
    Set alternative file pointer to an already open .chu file.
*/
inline
void
nChunkLodMesh::SetFile(Data::CFileStream* file)
{
    n_assert(0 == this->chuFile);
    this->chuFile = file;
    this->extChuFile = true;
}

//------------------------------------------------------------------------------
/**
    Get pointer to file object.
*/
inline
Data::CFileStream*
nChunkLodMesh::GetFile() const
{
    return this->chuFile;
}

//------------------------------------------------------------------------------
/**
    This sets the position of the chunk data inside the .chu file.

    @param  pos     position to read data from
*/
inline
void
nChunkLodMesh::SetFileLocation(int pos)
{
    this->fileChunkPos = pos;
}

//------------------------------------------------------------------------------
/**
    Returns position and size of data chunk inside .chu file.

    @return     position of data in .chu file
*/
inline
int
nChunkLodMesh::GetFileLocation()
{
    return this->fileChunkPos;
}

//------------------------------------------------------------------------------
/**
    Set vertex decompression parameters (sx, sy, sz).
    Terrain vertices are quantized in the terrain file. They need to be
    dequantized to be of any use.

    @param  s       vector3 with the (sx, sy, sz) params
*/
inline
void
nChunkLodMesh::SetVertexScale(const vector3& s)
{
    this->vertexScale = s;
}

//------------------------------------------------------------------------------
/**
    Returns the vertex decompression parameters.

    @return     the vertex decompression params
*/
inline
const vector3&
nChunkLodMesh::GetVertexScale() const
{
    return this->vertexScale;
}

//------------------------------------------------------------------------------
/**
    Sets the vertex xyz offset.

    Vertices are compressed in the terrain file. These values are 
	used to recreate them. Offset is given in the \a xz plane
	See nLodChunk::read for examples of how to obtain these values.

    @param  offset      a vector3 with the vertex offsets
*/
inline
void
nChunkLodMesh::SetVertexOffset(const vector3& offset)
{
    this->vertexOffset = offset;
}

//------------------------------------------------------------------------------
/**
    Returns the vertex xyz offset.
*/
inline
const vector3&
nChunkLodMesh::GetVertexOffset() const
{
    return this->vertexOffset;
}

//------------------------------------------------------------------------------
#endif

