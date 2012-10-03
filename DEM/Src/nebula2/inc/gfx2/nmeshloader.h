#ifndef N_MESHLOADER_H
#define N_MESHLOADER_H
//------------------------------------------------------------------------------
/**
    @class nMeshLoader
    @ingroup Gfx2

    Base class for specific mesh loaders. Mesh loaders load mesh data
    from a specific file format into user-provided memory blocks
    for vertex and index data.

    nMeshLoader is a non-functional base class. Use the specialized
    classes nN3d2Loader and nNvx2Loader to load mesh files of those
    formats.

    The following shows that how to read vertices and indices from .n3d2 file.

    @code
    // create mesh loader for a .n3d2 file.
    nMeshLoader* meshLoader = n_new (nN3d2Loader);

    // open specified .n3d2 file.
    meshLoader->SetIndexType(nMeshLoader::Index16);
    meshLoader->SetFilename(filename);
    meshLoader->Open();

    int numVertices = meshLoader->GetNumVertices();
    int vertexWidth = meshLoader->GetVertexWidth();
    int numTriangles = meshLoader->GetNumTriangles();

    // read vertices.
    const int bufferSize = numVertices * vertexWidth * sizeof(float);
    float* vbuf = n_new_array(float, bufferSize);

    meshLoader->ReadVertices(vbuf, bufferSize);

    int i;
    int idx = 0;
    vector3 v;
    for (i=0; i<numVertices; i++)
    {
        v.x = vbuf[idx];
        v.y = vbuf[idx + 1];
        v.z = vbuf[idx + 2];

        // Do some task whatever you want
        // ...

        idx += vertexWidth;
    }
    n_delete_array(vbuf);

    // read indices.
    ushort ibufSize = meshLoader->GetNumIndices() * sizeof(ushort);
    ushort* ibuf = n_new_array(ushort, ibufSize);

    meshLoader->ReadIndices(ibuf, ibufSize);

    idx = 0;
    ushort i0, i1, i2;
    for (i=0; i<numTriangles; i++)
    {
        i0 = ibuf[idx++];
        i1 = ibuf[idx++];
        i2 = ibuf[idx++];

        // Do some task whatever you want
        // ...
    }
    n_delete_array(ibuf);

    // close the file.
    meshLoader->Close();

    n_delete (meshLoader);
    @endcode

    (C) 2003 RadonLabs GmbH
*/
#include "kernel/ntypes.h"
#include "util/narray.h"
#include "gfx2/nmesh2.h"
#include <Data/Streams/FileStream.h>

//------------------------------------------------------------------------------
class nMeshLoader
{
public:
    /// index types
    enum IndexType
    {
        Index16,
        Index32,
    };

    /// constructor
    nMeshLoader();
    /// destructor
    virtual ~nMeshLoader();
    /// set filename
    void SetFilename(const nString& name);
    /// get filename
    const nString& GetFilename() const;
    /// 16 or 32 bit indices (default is 16)
    void SetIndexType(IndexType t);
    /// get index type
    IndexType GetIndexType() const;
    /// set the valid vertex components
    void SetValidVertexComponents(int components);
    /// get the valid vertex components
    int GetValidVertexComponents();
    /// open the file and read header data
    virtual bool Open();
    /// close the file
    virtual void Close();
    /// get number of groups (valid after Open())
    int GetNumGroups() const;
    /// get group info (valid after Open())
    const nMeshGroup& GetGroupAt(int index) const;
    /// get number of vertices (valid after Open())
    int GetNumVertices() const;
    /// get vertex width (aka stride) in sizeof(float)'s (valid after Open())
    int GetVertexWidth() const;
    /// get number of triangles (valid after Open())
    int GetNumTriangles() const;
    /// get number of indices (valid after Open())
    int GetNumIndices() const;
    /// get number of edges (valid after Open())
    int GetNumEdges() const;
    /// get vertex components (see gfx2/nmesh2.h)(valid after Open())
    int GetVertexComponents() const;
    /// read vertex data
    virtual bool ReadVertices(void* buffer, int bufferSize);
    /// read index data
    virtual bool ReadIndices(void* buffer, int bufferSize);
    /// read edge data
    virtual bool ReadEdges(void* buffer, int bufferSize);

protected:
    nString filename;
	Data::CFileStream* file;
    IndexType indexType;
    int numGroups;
    int numVertices;
    int vertexWidth;
    int fileVertexWidth;
    int numTriangles;
    int numIndices;
    int numEdges;
    int vertexComponents;
    int fileVertexComponents;
    int validVertexComponents;
    nArray<nMeshGroup> groupArray;
};

//------------------------------------------------------------------------------
/**
*/
inline
nMeshLoader::nMeshLoader() :
    file(0),
    indexType(Index16),
    numGroups(0),
    numVertices(0),
    vertexWidth(0),
    fileVertexWidth(0),
    numTriangles(0),
    numIndices(0),
    numEdges(0),
    vertexComponents(0),
    fileVertexComponents(0),
    validVertexComponents(0xFFFF)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
nMeshLoader::~nMeshLoader()
{
    n_assert(0 == this->file);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nMeshLoader::SetFilename(const nString& name)
{
    n_assert(name.IsValid());
    this->filename = name;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nString&
nMeshLoader::GetFilename() const
{
    return this->filename;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nMeshLoader::SetIndexType(IndexType t)
{
    this->indexType = t;
}

//------------------------------------------------------------------------------
/**
*/
inline
nMeshLoader::IndexType
nMeshLoader::GetIndexType() const
{
    return this->indexType;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nMeshLoader::SetValidVertexComponents(int components)
{
    n_assert(components);
    this->validVertexComponents = components;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMeshLoader::GetValidVertexComponents()
{
    return this->validVertexComponents;
}

//------------------------------------------------------------------------------
/**
    calculate the vertex components and vertex width for the current file
    and the set valid vertex components.
    the real Open() must be implemented in a sub class!
*/
inline
bool
nMeshLoader::Open()
{
    n_assert(0 != this->fileVertexWidth);
    n_assert(0 != this->fileVertexComponents);
    n_assert(0 != this->validVertexComponents);

    // create the real vertex components
    this->vertexComponents = (this->validVertexComponents & this->fileVertexComponents);
    this->vertexWidth = nMesh2::GetVertexWidthFromMask(this->vertexComponents);

    return true;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nMeshLoader::Close()
{
    n_error("nMeshLoader::Close() called!");
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMeshLoader::GetNumGroups() const
{
    return this->numGroups;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nMeshGroup&
nMeshLoader::GetGroupAt(int index) const
{
    return this->groupArray[index];
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMeshLoader::GetNumVertices() const
{
    return this->numVertices;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMeshLoader::GetVertexWidth() const
{
    return this->vertexWidth;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMeshLoader::GetNumTriangles() const
{
    return this->numTriangles;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMeshLoader::GetNumIndices() const
{
    return this->numIndices;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMeshLoader::GetNumEdges() const
{
    return this->numEdges;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMeshLoader::GetVertexComponents() const
{
    return this->vertexComponents;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nMeshLoader::ReadVertices(void* /*buffer*/, int /*bufferSize*/)
{
    n_error("nMeshLoader::ReadVertices() called!");
    return false;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nMeshLoader::ReadIndices(void* /*buffer*/, int /*bufferSize*/)
{
    n_error("nMeshLoader::ReadIndices() called!");
    return false;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nMeshLoader::ReadEdges(void* /*buffer*/, int /*bufferSize*/)
{
    n_error("nMeshLoader::ReadEdges() called!");
    return false;
}
//------------------------------------------------------------------------------
#endif
