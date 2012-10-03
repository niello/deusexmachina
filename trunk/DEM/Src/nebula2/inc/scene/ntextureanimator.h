#ifndef N_TEXTUREANIMATOR_H
#define N_TEXTUREANIMATOR_H
//------------------------------------------------------------------------------
/**
    @class nTextureAnimator
    @ingroup SceneAnimators

    @brief nTextureAnimator selects between one of several possible textures.

    See also @ref N2ScriptInterface_ntextureanimator

    (C) 2004 Rafael Van Daele-Hunt
*/
#include "scene/nanimator.h"
#include "util/narray.h"

//------------------------------------------------------------------------------
class nTextureAnimator : public nAnimator
{
public:
    /// constructor
    nTextureAnimator();
    /// destructor
    virtual ~nTextureAnimator();
    /// save object to persistent stream
    //virtual bool SaveCmds(nPersistServer* ps);

    /// return the type of this animator object
    virtual Type GetAnimatorType() const;
    /// called by scene node objects which wish to be animated by this object
    virtual void Animate(nSceneNode* sceneNode, nRenderContext* renderContext);

    /// Add a texture to the array (with index number equal to the number of textures added thus far)
    void AddTexture(const char* path);
    /// Sets the shader state parameter that will be passed to nAbstractShaderNode::SetTexture
    void SetShaderParam(const char* param);

private:
    /// Returns the number of textures that have been added so far
    int GetNumTextures() const;
    /// Returns the nth texture in the (zero based) array
    nTexture2* GetTextureAt(int n) const;
    /// a convenience function for SaveCmds
    const char* GetShaderParam() const;

    nArray<nRef<nTexture2> > textureArray;
    nShaderState::Param shaderParam;

};

//------------------------------------------------------------------------------
/**
*/
inline
nAnimator::Type
nTextureAnimator::GetAnimatorType() const
{
    return Shader;
}

#endif
