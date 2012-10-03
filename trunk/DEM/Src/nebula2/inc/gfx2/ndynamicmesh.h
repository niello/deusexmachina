#ifndef N_DYNAMICMESH_H
#define N_DYNAMICMESH_H
//------------------------------------------------------------------------------
/**
    @class nDynamicMesh
    @ingroup Gfx2

    Helper class for rendering dynamic geometry, simplifies writing
    to the global dynamic mesh offered by the gfx server.

    @section nDynamicMeshUsage Usage

    The first thing you should do to use a @ref nDynamicMesh is that initialization
    of the dynamic mesh object. This can be done by calling nDynamicMesh::Initialize().
    @code
    dynMesh.Initialize(nGfxServer2::TriangleList,
                       nMesh2::Coord |
                       nMesh2::Uv0 | nMesh2::Uv1 | nMesh2::Uv2 |
                       nMesh2::Color,
                       nMesh2::WriteOnly | nMesh2::NeedVertexShader,
                       false // shared mesh flag. false means a new mesh is created for this.
                      );
    @endcode

    Before filling the mesh, nDynamicMesh::Begin() or nDynamicMesh::BeginIndexed()
    should be called to lock the vertex (and index buffer if you use indexed
    dynamic mesh, the case nDynamicMesh::BeginIndexed() is used).
    @code
    float* dstVertices = 0; // Retrived vertex buffer pointer.
    int maxVertices    = 0; // Max number of vertices which retrived from created(or shared) mesh

    dynMesh.Begin(dstVertices, maxVertices);
    @endcode

    Now, you can fill vertices via retrieved vertex buffer pointer.
    @code
    dstVertices[curIndex++] = curPosition.x;
    dstVertices[curIndex++] = curPosition.y;
    dstVertices[curIndex++] = curPosition.z;
    ...
    @endcode

    During you fill vertices, you should check that the mesh is full.
    If it is, you should throw away aleady filled vertices by calling
    nDynamicMesh::Swap()(or nDynamicMesh::SwapIndexed() for indexed vertices)
    @code
    dynMesh.Swap(curVertex, dstVertices);
    @endcode

    After you fill all vertices, call nDynamicMesh::End() to render the mesh.
    @code
    dynMesh.End(curVertex);
    @endcode

    See nParticleEmitter::Render() for complete source code of this section.

    (C) 2003 RadonLabs GmbH
*/
#include "gfx2/ngfxserver2.h"
#include "gfx2/nmesh2.h"

//------------------------------------------------------------------------------
class nDynamicMesh
{
public:
    /// constructor
    nDynamicMesh();
    /// destructor
    ~nDynamicMesh();
    /// initialize the dynamic mesh
    bool Initialize(nGfxServer2::PrimitiveType primType, int vertexComponents, int usageFlags, bool indexedRendering, const nString& rsrcBaseName="dyn_", bool shared = true);
    /// if this returns false, call Initialize()
    bool IsValid() const;
    /// begin indexed rendering
    void BeginIndexed(float*& vertexPointer, ushort*& indexPointer, int& maxNumVertices, int& maxNumIndices);
    /// do an intermediate swap for indexed rendering
    void SwapIndexed(int numValidVertices, int numValidIndices, float*& vertexPointer, ushort*& indexPointer);
    /// end indexed rendering
    void EndIndexed(int numValidVertices, int numValidIndices);
    /// begin non-indexed rendering
    void Begin(float*& vertexPointer, int& maxNumVertices);
    /// do an intermediate swap for non-indexed rendering
    void Swap(int numValidVertices, float*& vertexPointer);
    /// end non-indexed rendering
    void End(int numValidVertices);

protected:
    enum
    {
        IndexBufferSize = 4096,                     // number of vertices
        VertexBufferSize  = 3 * IndexBufferSize,    // number of indices
    };
    bool indexedRendering;
    nRef<nMesh2> refMesh;
    nGfxServer2::PrimitiveType primitiveType;
};

//------------------------------------------------------------------------------
#endif
