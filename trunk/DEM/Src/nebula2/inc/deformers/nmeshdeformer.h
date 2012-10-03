#ifndef N_MESHDEFORMER_H
#define N_MESHDEFORMER_H
//------------------------------------------------------------------------------
/**
    @class nMeshDeformer
    @ingroup Deformers
    @brief Base class for mesh deformers running on the CPU. While using vertex
    shaders for mesh deformation is a good thing, it is not always an option,
    and may be too inflexible. Mesh deformers take one or more input meshes,
    plus deformer-specific and write their result to one output mesh.

    (C) 2004 RadonLabs GmbH
*/
#include "kernel/ntypes.h"
#include "util/narray.h"
#include "gfx2/nmesh2.h"

//------------------------------------------------------------------------------
class nMeshDeformer
{
public:
    /// constructor
    nMeshDeformer();
    /// destructor
    virtual ~nMeshDeformer();
    /// set single input mesh
    void SetInputMesh(nMesh2* mesh);
    /// get single input mesh
    nMesh2* GetInputMesh() const;
    /// set single output mesh
    void SetOutputMesh(nMesh2* mesh);
    /// get single output mesh
    nMesh2* GetOutputMesh() const;
    /// set source start index
    void SetStartVertex(int i);
    /// get start index
    int GetStartVertex() const;
    /// get number of vertices to process
    void SetNumVertices(int n);
    /// get number of vertices to process
    int GetNumVertices() const;
    /// perform mesh deformation
    virtual void Compute();

protected:
    nRef<nMesh2> refInputMesh;
    nRef<nMesh2> refOutputMesh;
    int startVertex;
    int numVertices;
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nMeshDeformer::SetInputMesh(nMesh2* mesh)
{
    this->refInputMesh = mesh;
}

//------------------------------------------------------------------------------
/**
*/
inline
nMesh2*
nMeshDeformer::GetInputMesh() const
{
    return this->refInputMesh.get_unsafe();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nMeshDeformer::SetOutputMesh(nMesh2* mesh)
{
    this->refOutputMesh = mesh;
}

//------------------------------------------------------------------------------
/**
*/
inline
nMesh2*
nMeshDeformer::GetOutputMesh() const
{
    return this->refOutputMesh.get_unsafe();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nMeshDeformer::SetStartVertex(int i)
{
    n_assert(i >= 0);
    this->startVertex = i;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMeshDeformer::GetStartVertex() const
{
    return this->startVertex;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nMeshDeformer::SetNumVertices(int n)
{
    n_assert(n > 0);
    this->numVertices = n;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMeshDeformer::GetNumVertices() const
{
    return this->numVertices;
}

//------------------------------------------------------------------------------
#endif
