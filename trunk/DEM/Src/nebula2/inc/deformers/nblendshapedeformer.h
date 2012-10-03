#ifndef N_BLENDSHAPEDEFORMER_H
#define N_BLENDSHAPEDEFORMER_H
//------------------------------------------------------------------------------
/**
    @class nBlendShapeDeformer
    @ingroup Deformers
    @brief A blend shape mesh deformer which blends several source meshes
    into one output mesh based on weights.

    (C) 2004 RadonLabs GmbH
*/
#include "deformers/nmeshdeformer.h"
#include "gfx2/nmesharray.h"

//------------------------------------------------------------------------------
class nBlendShapeDeformer : public nMeshDeformer
{
public:
    /// constructor
    nBlendShapeDeformer();
    /// destructor
    virtual ~nBlendShapeDeformer();
    /// set array of weights
    void SetWeights(const nArray<float>& weights);
    /// get array of weights
    const nArray<float>& GetWeights() const;
    /// set source mesh array
    void SetInputMeshArray(nMeshArray* meshArray);
    /// get source mesh array
    nMeshArray* GetInputMeshArray() const;
    /// perform mesh deformation
    virtual void Compute();

private:
    nRef<nMeshArray> refInputMeshArray;
    nArray<float> weightArray;
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nBlendShapeDeformer::SetWeights(const nArray<float>& weights)
{
    this->weightArray = weights;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nArray<float>&
nBlendShapeDeformer::GetWeights() const
{
    return this->weightArray;
}

//------------------------------------------------------------------------------
#endif
