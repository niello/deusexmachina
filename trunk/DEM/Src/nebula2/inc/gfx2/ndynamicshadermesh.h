#ifndef N_SHADERDYNAMICMESH_H
#define N_SHADERDYNAMICMESH_H
//------------------------------------------------------------------------------
/**
    @class nShaderDynamicMesh
    @ingroup Gfx2

    A Extension to the dynamic mesh, that takes a shader, and
    draws all needed passes.

    (C) 2004 RadonLabs GmbH
*/

#include "gfx2/ndynamicmesh.h"
#include "gfx2/nshader2.h"

//------------------------------------------------------------------------------
class nDynamicShaderMesh : public nDynamicMesh
{
public:
    /// constructor
    nDynamicShaderMesh();
    /// destructor
    ~nDynamicShaderMesh();
    /// if this returns false, call Initialize() and set the shader!
    bool IsValid() const;
    /// set shader
    void SetShader(nShader2* shader);
    /// get shader
    nShader2* GetShader() const;

    //FIXME
    /*
    /// begin indexed rendering
    void BeginIndexed(float*& vertexPointer, ushort*& indexPointer, int& maxNumVertices, int& maxNumIndices);
    /// do an intermediate swap for indexed rendering
    void SwapIndexed(int numValidVertices, int numValidIndices, float*& vertexPointer, ushort*& indexPointer);
    /// end indexed rendering
    void EndIndexed(int numValidVertices, int numValidIndices);
    */

    /// begin non-indexed rendering
    void Begin(float*& vertexPointer, int& maxNumVertices);
    /// do an intermediate swap for non-indexed rendering
    void Swap(int numValidVertices, float*& vertexPointer);
    /// end non-indexed rendering
    void End(int numValidVertices);

private:
    nRef<nShader2> refShader;
    int numPasses;
};

//------------------------------------------------------------------------------
#endif
