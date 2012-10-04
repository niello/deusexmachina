//------------------------------------------------------------------------------
//  nrpshader.cc
//  (C) 2004 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "renderpath/nrpshader.h"
#include "gfx2/ngfxserver2.h"

//------------------------------------------------------------------------------
/**
*/
nRpShader::nRpShader() :
    bucketIndex(-1)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nRpShader::~nRpShader()
{
    if (this->refShader.isvalid())
    {
        this->refShader->Release();
        this->refShader.invalidate();
    }
}

//------------------------------------------------------------------------------
/**
    Validates the embedded shader.
*/
void
nRpShader::Validate()
{
    nShader2* shd = 0;
    if (!this->refShader.isvalid())
    {
        n_assert(!this->filename.IsEmpty());
        shd = nGfxServer2::Instance()->NewShader(this->filename);
        this->refShader = shd;
    }
    else
    {
        shd = this->refShader;
    }

    n_assert(shd);
    if (!shd->IsLoaded())
    {
        shd->SetFilename(this->filename);
        if (!shd->Load())
        {
            shd->Release();
            n_error("nRpShader: could not load shader file '%s'!", this->filename.Get());
        }
    }
}
