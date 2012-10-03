#ifndef N_BLENDSHAPERENDERER_H
#define N_BLENDSHAPERENDERER_H
//------------------------------------------------------------------------------
/**
    @class nBlendShapeRenderer
    @ingroup Deformers
    @brief An unified blend shape renderer (either blends on the CPU or in the
    vertex shader).

    (C) 2004 RadonLabs GmbH
*/
#include "kernel/ntypes.h"
#include "gfx2/ndynamicmesh.h"

//------------------------------------------------------------------------------
class nBlendShapeRenderer
{
public:
    /// constructor
    nBlendShapeRenderer();
    /// destructor
    ~nBlendShapeRenderer();
    /// initialize the blend shape renderer
    bool Initialize(bool cpuBlending, nMeshArray* srcMeshArray);
    /// return true if initialized
    bool IsInitialized() const;
    /// render
    void Render(int meshGroupIndex, const nArray<float>& weights);

private:
    /// internal setup routine
    void Setup();
    /// render with CPU blending
    void RenderCpuBlending(int meshGroupIndex, const nArray<float>& weights);
    /// render with vertex blending
    void RenderShaderBlending(int meshGroupIndex, const nArray<float>& weights);

    bool initialized;
    bool useCpuBlending;
    nRef<nMesh2> refDstMesh;            // optional, only when cpu blending
    nRef<nMeshArray> refSrcMeshArray;
};

//------------------------------------------------------------------------------
/**
*/
inline
bool
nBlendShapeRenderer::IsInitialized() const
{
    return this->initialized;
}

//------------------------------------------------------------------------------
#endif
