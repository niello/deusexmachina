//------------------------------------------------------------------------------
//  ntextureanimator_main.cc
//  (C) 2004 Rafael Van Daele-Hunt
//------------------------------------------------------------------------------
#include "scene/ntextureanimator.h"
#include "scene/nrendercontext.h"
#include "scene/nabstractshadernode.h"
#include "gfx2/ngfxserver2.h"

nNebulaClass(nTextureAnimator, "nanimator");

//------------------------------------------------------------------------------
/**
*/
nTextureAnimator::nTextureAnimator() :
    shaderParam(nShaderState::InvalidParameter)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nTextureAnimator::~nTextureAnimator()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    This does the actual work of selecting a texture.

    @param  sceneNode       object to texture (must be of class nAbstractShaderNode)
    @param  renderContext   current render context
*/
void
nTextureAnimator::Animate(nSceneNode* sceneNode, nRenderContext* renderContext)
{
    n_assert(sceneNode);
    n_assert(sceneNode->IsA("nabstractshadernode"));
    n_assert(renderContext);
    n_assert(nVariable::InvalidHandle != this->channelVarHandle);
    n_assert(nShaderState::InvalidParameter != this->shaderParam);

    // get the active texture index from the render context
    nVariable* var = renderContext->GetVariable(this->channelVarHandle);
    n_assert(var);
    int texIndex = var->GetInt();
    n_assert(texIndex < this->GetNumTextures());

    renderContext->GetShaderOverrides().SetArg(this->shaderParam, nShaderArg(GetTextureAt(texIndex)));
}

void
nTextureAnimator::AddTexture(const char* path)
{
    nRef<nTexture2> refTexture(nGfxServer2::Instance()->NewTexture(path));
    refTexture->SetFilename(path);
    refTexture->Load();
    this->textureArray.PushBack(refTexture);
}

void
nTextureAnimator::SetShaderParam(const char* param)
{
    this->shaderParam = nShaderState::StringToParam(param);
    n_assert(nShaderState::InvalidParameter != this->shaderParam);
}

int
nTextureAnimator::GetNumTextures() const
{
    return this->textureArray.Size();
}

nTexture2*
nTextureAnimator::GetTextureAt(int n) const
{
    nTexture2* retVal = 0;
    if (n < this->GetNumTextures())
    {
        retVal = this->textureArray[n].get();
    }
    return retVal;
}

const char*
nTextureAnimator::GetShaderParam() const
{
    return nShaderState::ParamToString(this->shaderParam);
}
