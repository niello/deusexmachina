#ifndef N_CHARSKINRENDERER_H
#define N_CHARSKINRENDERER_H
//------------------------------------------------------------------------------
/**
    @class nCharSkinRenderer
    @ingroup Character

    @brief A smooth character skin renderer. Takes a pointer to an nMesh2
    object which contains the skin mesh, and a pointer to an
    up to date nCharSkeleton object.

    (C) 2002 RadonLabs GmbH
*/

#include "kernel/ntypes.h"
#include "character/ncharskeleton.h"
#include "character/ncharjointpalette.h"
#include "gfx2/nmesh2.h"

//-----------------------------------------------------------------------------
class nCharSkinRenderer
{
public:
    /// constructor
    nCharSkinRenderer();
    /// destructor
    ~nCharSkinRenderer();
    /// initialize the char skin renderer
    bool Initialize(nMesh2* srcMesh);
    /// return true if renderer is in the initialized state
    bool IsInitialized() const;
    /// begin rendering the skin
    void Begin(const nCharSkeleton* skel);
    /// render a single skin fragment
    void Render(int meshGroupIndex, nCharJointPalette& jointPalette);
    /// finish rendering
    void End();

private:
    /// render with vertex skinning
    void RenderShaderSkinning(int meshGroupIndex, nCharJointPalette& jointPalette);

    bool initialized;
    bool inBegin;
    nRef<nMesh2> refSrcMesh;        // the source mesh
    const nCharSkeleton* charSkeleton;
};

//------------------------------------------------------------------------------
/**
*/
inline
bool
nCharSkinRenderer::IsInitialized() const
{
    return this->initialized;
}

//------------------------------------------------------------------------------
#endif
