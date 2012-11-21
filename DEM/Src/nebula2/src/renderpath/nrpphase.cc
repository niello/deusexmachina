//------------------------------------------------------------------------------
//  nrpphase.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "renderpath/nrpphase.h"
#include "gfx2/ngfxserver2.h"
#include <Render/FrameShader.h>

//------------------------------------------------------------------------------
/**
*/
nRpPhase::nRpPhase() :
    inBegin(false),
    rpShaderIndex(-1),
    sortingOrder(FrontToBack),
    lightMode(Off),
    pFrameShader(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
int
nRpPhase::Begin()
{
    n_assert(!this->inBegin);
    n_assert(-1 != this->rpShaderIndex);
    n_assert(this->pFrameShader);

// IDirect3DDevice9::SetRenderTarget resets the scissor rectangle to the full render target,
// analogous to the viewport reset. IDirect3DDevice9::SetScissorRect is recorded by stateblocks,
// and IDirect3DDevice9::CreateStateBlock with the all state setting (D3DSBT_ALL value in D3DSTATEBLOCKTYPE).
// The scissor test also affects the device IDirect3DDevice9::Clear operation.
	//// Set fullscreen scissors
	//nGfxServer2::Instance()->SetScissorRect(rectangle(vector2::zero, vector2(1.0f, 1.0f)));

    // note: save/restore state for phase shaders!
    nShader2* shd = this->pFrameShader->shaders[this->rpShaderIndex].GetShader();
    if (!technique.IsEmpty()) shd->SetTechnique(this->technique.Get());
    nGfxServer2::Instance()->SetShader(shd);
    int numShaderPasses = shd->Begin(true);
    n_assert(1 == numShaderPasses); // assume 1-pass phase shader!
    shd->BeginPass(0);

    this->inBegin = true;
    return this->sequences.Size();
}

//------------------------------------------------------------------------------
/**
*/
void
nRpPhase::End()
{
    n_assert(this->inBegin);
    n_assert(-1 != this->rpShaderIndex);

    nShader2* shd = this->pFrameShader->shaders[this->rpShaderIndex].GetShader();
    shd->EndPass();
    shd->End();

    this->inBegin = false;
}

//------------------------------------------------------------------------------
/**
*/
void
nRpPhase::Validate()
{
    n_assert(this->pFrameShader);

    // invoke validate on sequences
    int i;
    int num = this->sequences.Size();
    for (i = 0; i < num; i++)
    {
        this->sequences[i].SetRenderPath(this->pFrameShader);
        this->sequences[i].Validate();
    }

    // find shader
    if (-1 == this->rpShaderIndex)
    {
        n_assert(!this->shaderAlias.IsEmpty());
        this->rpShaderIndex = this->pFrameShader->FindShaderIndex(this->shaderAlias);
        if (-1 == this->rpShaderIndex)
        {
            n_error("nRpPhase::Validate(): couldn't find shader alias '%s' in render path xml file!", this->shaderAlias.Get());
        }
    }
}


