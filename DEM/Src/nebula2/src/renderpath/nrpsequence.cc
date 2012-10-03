//------------------------------------------------------------------------------
//  nrpsequence.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "renderpath/nrpsequence.h"
#include "renderpath/nrenderpath2.h"
#include "gfx2/ngfxserver2.h"

//------------------------------------------------------------------------------
/**
*/
nRpSequence::nRpSequence() :
    renderPath(0),
    rpShaderIndex(-1),
    firstLightAlphaEnabled(false),
    shaderUpdatesEnabled(true),
    mvpOnly(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nRpSequence::~nRpSequence()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
int
nRpSequence::GetShaderBucketIndex() const
{
    n_assert(-1 != this->rpShaderIndex);
    n_assert(this->renderPath);
    return this->renderPath->GetShader(this->rpShaderIndex).GetBucketIndex();
}

//------------------------------------------------------------------------------
/**
*/
void
nRpSequence::Validate()
{
    n_assert(this->renderPath);

    // setup profiler
    #if __NEBULA_STATS__
    /*
    n_assert(this->section);
    n_assert(this->pass);
    n_assert(this->phase);
    if (!this->prof.IsValid())
    {
        nString n;
        n.Format("profRpSequence_%s_%s_%s_%s", this->section->GetName().Get(), this->pass->GetName().Get(), this->phase->GetName().Get(), this->shaderAlias.Get());
        this->prof.Initialize(n.Get());
    }
    */
    #endif

    if (-1 == this->rpShaderIndex)
    {
        n_assert(!this->shaderAlias.IsEmpty());
        this->rpShaderIndex = this->renderPath->FindShaderIndex(this->shaderAlias);
        if (-1 == this->rpShaderIndex)
        {
            n_error("nRpSequence::Validate(): couldn't find shader alias '%s' in render path xml file!", this->shaderAlias.Get());
        }
    }
}

//------------------------------------------------------------------------------
/**
    NOTE: sequence shaders generally do NOT backup the current state.
*/
int
nRpSequence::Begin()
{
    n_assert(-1 != this->rpShaderIndex);
    n_assert(this->renderPath);

    #if __NEBULA_STATS__
    //this->prof.Start();
    #endif

    nGfxServer2::Instance()->SetHint(nGfxServer2::MvpOnly, this->mvpOnly);
    nShader2* shd = this->renderPath->GetShader(this->rpShaderIndex).GetShader();
    if (this->shaderUpdatesEnabled)
    {
        this->UpdateVariableShaderParams();
        shd->SetParams(this->shaderParams);
    }
    if (!this->technique.IsEmpty())
    {
        shd->SetTechnique(this->technique.Get());
    }
    return shd->Begin(false);
}

//------------------------------------------------------------------------------
/**
*/
void
nRpSequence::BeginPass(int pass)
{
    n_assert(-1 != this->rpShaderIndex);
    n_assert(pass >= 0);
    n_assert(this->renderPath);

    this->renderPath->GetShader(this->rpShaderIndex).GetShader()->BeginPass(pass);
}

//------------------------------------------------------------------------------
/**
*/
void
nRpSequence::EndPass()
{
    n_assert(-1 != this->rpShaderIndex);
    n_assert(this->renderPath);
    this->renderPath->GetShader(this->rpShaderIndex).GetShader()->EndPass();
}

//------------------------------------------------------------------------------
/**
    NOTE: currently, the previous technique is not restored (should this
    even be the intended behavior???)
*/
void
nRpSequence::End()
{
    n_assert(-1 != this->rpShaderIndex);
    n_assert(this->renderPath);
    this->renderPath->GetShader(this->rpShaderIndex).GetShader()->End();
    nGfxServer2::Instance()->SetHint(nGfxServer2::MvpOnly, false);

    #if __NEBULA_STATS__
    //this->prof.Stop();
    #endif
}

//------------------------------------------------------------------------------
/**
    This gathers the current global variable values from the render path
    object and updates the shader parameter block with the new values.
*/
void
nRpSequence::UpdateVariableShaderParams()
{
    // for each variable shader parameter...
    int varIndex;
    int numVars = this->varContext.GetNumVariables();
    for (varIndex = 0; varIndex < numVars; varIndex++)
    {
        const nVariable& paramVar = this->varContext.GetVariableAt(varIndex);

        // get shader state from variable
        nShaderState::Param shaderParam = (nShaderState::Param) paramVar.GetInt();

        // get the current value
        const nVariable* valueVar = nVariableServer::Instance()->GetGlobalVariable(paramVar.GetHandle());
        n_assert(valueVar);
        nShaderArg shaderArg;
        switch (valueVar->GetType())
        {
            case nVariable::Int:
                shaderArg.SetInt(valueVar->GetInt());
                break;

            case nVariable::Float:
                shaderArg.SetFloat(valueVar->GetFloat());
                break;

            case nVariable::Float4:
                shaderArg.SetFloat4(valueVar->GetFloat4());
                break;

            case nVariable::Object:
                shaderArg.SetTexture((nTexture2*) valueVar->GetObj());
                break;

            case nVariable::Matrix:
                shaderArg.SetMatrix44(&valueVar->GetMatrix());
                break;

            case nVariable::Vector4:
                shaderArg.SetVector4(valueVar->GetVector4());
                break;

            default:
                n_error("nRpPass: Invalid shader arg datatype!");
                break;
        }

        // update the shader parameter
        this->shaderParams.SetArg(shaderParam, shaderArg);
    }
}
