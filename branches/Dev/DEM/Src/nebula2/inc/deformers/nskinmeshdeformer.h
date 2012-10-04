#ifndef N_SKINMESHDEFORMER_H
#define N_SKINMESHDEFORMER_H
//------------------------------------------------------------------------------
/**
    @class nSkinMeshDeformer
    @ingroup Deformers
    @brief A mesh deformer which implements smooth vertex skinning on the CPU.

    Normally this is done in the vertex shader, but there are cases where
    it's useful to have it on the CPU. To use it, just set an input mesh
    (needs joint indices and vertex weights components), set output mesh
    (needs identical components as input mesh, but without joint indices
    and vertex weights), set a character skeleton and a joint palette,
    and call Compute().

    (C) 2004 RadonLabs GmbH
*/
#include "deformers/nmeshdeformer.h"
#include "character/ncharskeleton.h"
#include "character/ncharjointpalette.h"

//------------------------------------------------------------------------------
class nSkinMeshDeformer : public nMeshDeformer
{
public:
    /// constructor
    nSkinMeshDeformer();
    /// destructor
    virtual ~nSkinMeshDeformer();
    /// set pointer to a character skeleton
    void SetCharSkeleton(const nCharSkeleton* skel);
    /// get pointer to a character skeleton
    const nCharSkeleton* GetCharSkeleton() const;
    /// set pointer to joint palette
    void SetJointPalette(nCharJointPalette* pal);
    /// get pointer to joint palette
    nCharJointPalette* GetJointPalette() const;
    /// perform deformation
    virtual void Compute();

protected:
    const nCharSkeleton* skeleton;
    nCharJointPalette* jointPalette;
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nSkinMeshDeformer::SetCharSkeleton(const nCharSkeleton* skel)
{
    n_assert(skel);
    this->skeleton = skel;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nCharSkeleton*
nSkinMeshDeformer::GetCharSkeleton() const
{
    return this->skeleton;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nSkinMeshDeformer::SetJointPalette(nCharJointPalette* pal)
{
    n_assert(pal);
    this->jointPalette = pal;
}

//------------------------------------------------------------------------------
/**
*/
inline
nCharJointPalette*
nSkinMeshDeformer::GetJointPalette() const
{
    return this->jointPalette;
}

//------------------------------------------------------------------------------
#endif
