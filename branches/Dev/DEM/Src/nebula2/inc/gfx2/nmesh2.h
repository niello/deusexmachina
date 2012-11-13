#ifndef N_MESH2_H
#define N_MESH2_H
//------------------------------------------------------------------------------
/**
    @class nMesh2
    @ingroup Gfx2

    - 25-Sep-04     floh    I have removed the refill buffers mode, see
                            the nResource class for proper handling of externally
                            filled resource objects

    Internally holds opaque vertex and index data to feed a vertex shader.
    Vertices in a mesh are simply an array of floats.
    Meshes are generally static and loaded from mesh resource files.

    nMesh2 is normally a superclass for Gfx API specific derived classes, like
    Direct3D(see @ref nD3D9Mesh) or OpenGL.

    <h2>Mesh Group</h2>

    A mesh can be consist of several groups. If a mesh has more than one group,
    you need a shapenode for each mesh group you want to render.
    The reason for this is that if you've split a mesh into groups then
    you must have some reason for doing that such as rendering them with different
    materials/shaders/textures or transforming them differently and doing that
    requires seperate shapenodes.

    If you have one mesh file <tt>mymesh.n3d2</tt> and the mesh file has two groups
    due to it has different textures for each group then you need two shapenode to
    render it. The following script illustarates it:
    @verbatim
    new nshapenode shape0
        sel shape0
        ...
        .setmesh "meshes:mymesh.n3d2"
        .setgroupindex 0
        .settexture "DiffMap0" "textures:tex01.dds"
    sel ..
    new nshapenode shape1
        sel shape1
        ...
        .setmesh "meshes:mymesh.n3d2"
        .setgroupindex 1
        .settexture "DiffMap0" "textures:tex02.dds"
    sel ..
    @endverbatim

    <h2>Mesh File Format</h2>

    ASCII Mesh Fileformats(.n3d2):

    @verbatim
    type n3d2
    numgroups [numGroups]
    numvertices [numVertices]
    vertexwidth [vertexWidth]
    numtris [numTriangles]
    numedges [numEdges] - could be 0
    vertexcomps [coord normal uv0 uv1 uv2 uv3 color tangent binormal weights jindices]
    g [firstVertex] [numVertices] [firstTriangle] [numTriangles] [firstEdge] [numEdges]
    ...
    v 0.0 0.0 0.0 ....
    v 0.0 0.0 0.0 ....
    ...
    t 0 1 2
    t 1 2 3
    ....
    optional:
    e 0 1 0 1
    e 1 2 1 2
    ....
    @endverbatim

    Binary Mesh Fileformat(.nvx2):

    @verbatim
    uint magic = 'NVX2'
    int numGroups;
    int numVertices;
    int vertexWidth;
    int numTriangles;
    int numEdges
    int vertexComponents:   one bit set for each vertex component
        Coord    = (1<<0)
        Normal   = (1<<1)
        Uv0      = (1<<2)
        Uv1      = (1<<3)
        Uv2      = (1<<4)
        Uv3      = (1<<5)
        Color    = (1<<6)
        Tangent  = (1<<7)
        Binormal = (1<<8)
        ...

    for each group...
        int vertexRangeFirst
        int vertexRangeNum
        int firstTriangle
        int numTriangles
        int firstEdge
        int numEdges
    end

    float[] vertices;
    ushort[] indices;
    ushort[] edge [ 2 * faceIndex, 2 * vertexIndex ]
    @endverbatim

    (C) 2002 RadonLabs GmbH
*/
#include "resource/nresource.h"
#include "gfx2/ngfxserver2.h"

class nGfxServer2;
class nVariableServer;
class nMeshLoader;

struct nMeshGroup
{
	int		FirstVertex;
	int		NumVertices;
	int		FirstIndex;
	int		NumIndices;
	bbox3	Box;

	nMeshGroup(): FirstVertex(0), NumVertices(0), FirstIndex(0), NumIndices(0) {}
};

class nMesh2: public nResource
{
public:
    enum VertexComponent
    {
        Coord    = (1<<0),
        Normal   = (1<<1),
        Uv0      = (1<<2),
        Uv1      = (1<<3),
        Uv2      = (1<<4),
        Uv3      = (1<<5),
        Color    = (1<<6),
        Tangent  = (1<<7),
        Binormal = (1<<8),
        Weights  = (1<<9),
        JIndices = (1<<10),
        Coord4   = (1<<11),

        NumVertexComponents = 12,
        AllComponents = ((1<<NumVertexComponents) - 1),
    };

    enum Usage
    {
        // read/write behavior (mutually exclusive)
        WriteOnce = (1<<0),     ///< (default) CPU only fills the vertex buffer once, and never touches it again
        ReadOnly  = (1<<1),     ///< CPU reads from the vertex buffer, which can never be rendered
        WriteOnly = (1<<2),     ///< CPU writes frequently to vertex buffer, but never read data back
        ReadWrite = (1<<3),     ///< CPU writes and reads the vertex buffer, which is also rendered

        // use as point sprite buffer?
        PointSprite = (1<<6),

        // needs vertex shader?
        NeedsVertexShader = (1<<7),
    };

    enum OptimizationFlag
    {
        Faces       = (1<<0),
        Vertices    = (1<<1),
        AllOptimizations = Faces | Vertices
    };

    enum
    {
        InvalidIndex = 0xffff, // invalid index constant
    };

    /// constructor
    nMesh2();
    /// destructor
	virtual ~nMesh2() { if (!IsUnloaded()) Unload(); }

	virtual bool CreateNew(DWORD VtxCount, DWORD IdxCount, int Usage, int VtxComponents);

    /// lock vertex buffer
	virtual float* LockVertices() { return NULL; }
    /// unlock vertex buffer
	virtual void UnlockVertices() {}
    /// lock index buffer
    virtual ushort* LockIndices() { return NULL; }
    /// unlock index buffer
	virtual void UnlockIndices() {}

    /// set the mesh use type (both vertex and index buffer)
    void SetUsage(int useFlags);
    /// get the mesh use type (always returns vertex buffer usage flags
    int GetUsage() const;
    /// set usage type for vertex buffer
    void SetVertexUsage(int useFlags);
    /// get usage type for vertex buffer
    int GetVertexUsage() const;
    /// set usage type for index buffer
    void SetIndexUsage(int useFlags);
    /// get usage type for index buffer
    int GetIndexUsage() const;
    /// set number of vertices
    void SetNumVertices(int num);
    /// get number of vertices in mesh
    int GetNumVertices() const;
    /// set number of indices
    void SetNumIndices(int num);
    /// get num indices in mesh
    int GetNumIndices() const;
    /// set vertex components
    void SetVertexComponents(int compMask);
    /// get vertex components
    int GetVertexComponents() const;
    /// return true if the mesh has all of the given vertex components (OR'ed together)
    bool HasAllVertexComponents(int compMask) const;
    /// get the stride distance between the begin of a element and the requested component
    int GetVertexComponentOffset(VertexComponent component) const;
    /// get vertex width (number of floats in one vertex)
    int GetVertexWidth() const;
    /// get the vertex width for a given component mask
    static int GetVertexWidthFromMask(int compMask);
    /// set number of groups
    void SetNumGroups(int num);
    /// get number of groups
    int GetNumGroups() const;
    /// access group by index
    nMeshGroup& Group(int index) const;
    /// returns the byte size of the embedded vertex buffer
    int GetVertexBufferByteSize() const;
    /// returns the byte size of the embedded index buffer
    int GetIndexBufferByteSize() const;
    /// returns the byte size of the embedded edge buffer
    int GetEdgeBufferByteSize() const;
    /// get an estimated byte size of the resource data (for memory statistics)
	virtual int GetByteSize() { return 0; }

    /// optimize the mesh (can be redefined for each platform)
	virtual bool OptimizeMesh(OptimizationFlag flags, float * vertices, int numVertices, ushort * indices, int numIndices) { return true; }

    #ifndef NGAME
    /// enable/disable mesh optimization
    static bool optimizeMesh;
    #endif//NGAME

protected:
    /// load mesh resource
    virtual bool LoadResource();
    /// unload mesh resource
    virtual void UnloadResource();
    /// overload in subclass: create the vertex buffer
    virtual void CreateVertexBuffer();
    /// overload in subclass: create the index buffer
    virtual void CreateIndexBuffer();
    /// set the byte size of the vertex buffer
    void SetVertexBufferByteSize(int s);
    /// set the byte size of the index buffer
    void SetIndexBufferByteSize(int s);
    /// update the group bounding boxes (slow!)
    void UpdateGroupBoundingBoxes(float* vertexBufferData, ushort* indexBufferData);
    /// load file with the provided mesh loader
    bool LoadFile(nMeshLoader* meshLoader);

    int vertexUsage;
    int indexUsage;
    int vertexComponentMask;
    int vertexWidth;                // depends on vertexComponentMask
    int numVertices;
    int numIndices;
    int numGroups;
    nMeshGroup* groups;
    int vertexBufferByteSize;
    int indexBufferByteSize;
};

//------------------------------------------------------------------------------
/**
*/
inline
bool
nMesh2::HasAllVertexComponents(int compMask) const
{
    return (compMask == (this->vertexComponentMask & compMask));
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nMesh2::SetUsage(int useFlags)
{
    this->vertexUsage = useFlags;
    this->indexUsage  = useFlags;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMesh2::GetUsage() const
{
    return this->vertexUsage;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nMesh2::SetVertexUsage(int useFlags)
{
    this->vertexUsage = useFlags;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMesh2::GetVertexUsage() const
{
    return this->vertexUsage;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nMesh2::SetIndexUsage(int useFlags)
{
    this->indexUsage = useFlags;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMesh2::GetIndexUsage() const
{
    return this->indexUsage;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nMesh2::SetNumVertices(int num)
{
    this->numVertices = num;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMesh2::GetNumVertices() const
{
    return this->numVertices;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nMesh2::SetNumGroups(int num)
{
    n_assert(num > 0);
    n_assert(0 == this->groups);
    this->numGroups = num;
    this->groups = n_new_array(nMeshGroup,num);
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMesh2::GetNumGroups() const
{
    return this->numGroups;
}

//------------------------------------------------------------------------------
/**
*/
inline
nMeshGroup&
nMesh2::Group(int i) const
{
    n_assert((i >= 0) && (i < this->numGroups));
    return this->groups[i];
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nMesh2::SetVertexComponents(int compMask)
{
    this->vertexComponentMask = compMask & AllComponents;
    this->vertexWidth = nMesh2::GetVertexWidthFromMask(compMask);
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMesh2::GetVertexWidthFromMask(int compMask)
{
    int width = 0;
    if (compMask & Coord)    width += 3;
    if (compMask & Normal)   width += 3;
    if (compMask & Uv0)      width += 2;
    if (compMask & Uv1)      width += 2;
    if (compMask & Uv2)      width += 2;
    if (compMask & Uv3)      width += 2;
    if (compMask & Color)    width += 4;
    if (compMask & Tangent)  width += 3;
    if (compMask & Binormal) width += 3;
    if (compMask & Weights)  width += 4;
    if (compMask & JIndices) width += 4;
    if (compMask & Coord4)   width += 4;

    return width;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMesh2::GetVertexComponentOffset(VertexComponent component) const
{
    int ret = 0;
    if ((Coord == component) || (Coord4 == component)) return ret;
    if (this->vertexComponentMask & Coord) ret += 3;
    else if (this->vertexComponentMask & Coord4) ret += 4;

    if (Normal == component) return ret;
    if (this->vertexComponentMask & Normal) ret += 3;

    if (Uv0 == component) return ret;
    if (this->vertexComponentMask & Uv0) ret += 2;

    if (Uv1 == component) return ret;
    if (this->vertexComponentMask & Uv1) ret += 2;

    if (Uv2 == component) return ret;
    if (this->vertexComponentMask & Uv2) ret += 2;

    if (Uv3 == component) return ret;
    if (this->vertexComponentMask & Uv3) ret += 2;

    if (Color == component) return ret;
    if (this->vertexComponentMask & Color) ret += 4;

    if (Tangent == component) return ret;
    if (this->vertexComponentMask & Tangent) ret += 3;

    if (Binormal == component) return ret;
    if (this->vertexComponentMask & Binormal) ret += 3;

    if (Weights == component) return ret;
    if (this->vertexComponentMask & Weights) ret += 4;

    if (JIndices == component) return ret;
    if (this->vertexComponentMask & JIndices) ret += 4;

    // add more components here
    n_error("Requested component('%i') was not found!\n", (int) component);
    return -1;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMesh2::GetVertexComponents() const
{
    return this->vertexComponentMask;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMesh2::GetVertexWidth() const
{
    return this->vertexWidth;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nMesh2::SetNumIndices(int num)
{
    this->numIndices = num;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMesh2::GetNumIndices() const
{
    return this->numIndices;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nMesh2::SetVertexBufferByteSize(int s)
{
    n_assert(s > 0);
    this->vertexBufferByteSize = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nMesh2::SetIndexBufferByteSize(int s)
{
    n_assert(s > 0);
    this->indexBufferByteSize = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMesh2::GetVertexBufferByteSize() const
{
    return this->vertexBufferByteSize;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMesh2::GetIndexBufferByteSize() const
{
    return this->indexBufferByteSize;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nMesh2::CreateVertexBuffer()
{
    n_error("nMesh2: pure virtual function CreateVertexBuffer() called!\n");
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nMesh2::CreateIndexBuffer()
{
    n_error("nMesh2: pure virtual function CreateIndexBuffer() called!\n");
}

//------------------------------------------------------------------------------
#endif
