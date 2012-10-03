//------------------------------------------------------------------------------
//  nrpphase.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "renderpath/nrpphase.h"
#include "renderpath/nrenderpath2.h"
#include "gfx2/ngfxserver2.h"

//------------------------------------------------------------------------------
/**
*/
nRpPhase::nRpPhase() :
    inBegin(false),
    rpShaderIndex(-1),
    sortingOrder(FrontToBack),
    lightMode(Off),
    renderPath(0)
#if __NEBULA_STATS__
    ,section(0),
    pass(0)
#endif
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nRpPhase::~nRpPhase()
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
    n_assert(this->renderPath);
    nGfxServer2* gfxServer = nGfxServer2::Instance();

    #if __NEBULA_STATS__
    this->prof.Start();
    #endif

    // reset the scissor rect to full screen
    static const rectangle fullScreenRect(vector2(0.0f, 0.0f), vector2(1.0f, 1.0f));
    gfxServer->SetScissorRect(fullScreenRect);

    // note: save/restore state for phase shaders!
    nShader2* shd = this->renderPath->GetShader(this->rpShaderIndex).GetShader();
    if (!this->technique.IsEmpty())
    {
        shd->SetTechnique(this->technique.Get());
    }
    gfxServer->SetShader(shd);
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

    nShader2* shd = this->renderPath->GetShader(this->rpShaderIndex).GetShader();
    shd->EndPass();
    shd->End();

    this->inBegin = false;

    #if __NEBULA_STATS__
    this->prof.Stop();
    #endif
}

//------------------------------------------------------------------------------
/**
*/
void
nRpPhase::Validate()
{
    n_assert(this->renderPath);

    // setup profiler
    #if __NEBULA_STATS__
    n_assert(this->section);
    n_assert(this->pass);

    if (!this->prof.IsValid())
    {
        nString n;
        n.Format("profRpPhase_%s_%s_%s", this->section->GetName().Get(), this->pass->GetName().Get(), this->name.Get());
        this->prof.Initialize(n.Get());
    }
    #endif

    // invoke validate on sequences
    int i;
    int num = this->sequences.Size();
    for (i = 0; i < num; i++)
    {
        this->sequences[i].SetRenderPath(this->renderPath);
    #if __NEBULA_STATS__
        this->sequences[i].SetSection(this->section);
        this->sequences[i].SetPass(this->pass);
        this->sequences[i].SetPhase(this);
    #endif
        this->sequences[i].Validate();
    }

    // find shader
    if (-1 == this->rpShaderIndex)
    {
        n_assert(!this->shaderAlias.IsEmpty());
        this->rpShaderIndex = this->renderPath->FindShaderIndex(this->shaderAlias);
        if (-1 == this->rpShaderIndex)
        {
            n_error("nRpPhase::Validate(): couldn't find shader alias '%s' in render path xml file!", this->shaderAlias.Get());
        }
    }
}


