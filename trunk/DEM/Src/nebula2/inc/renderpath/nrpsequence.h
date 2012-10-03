#ifndef N_RPSEQUENCE_H
#define N_RPSEQUENCE_H
//------------------------------------------------------------------------------
/**
    @class nRpSequence
    @ingroup RenderPath
    @brief Encapsulates a sequence shader in a render path. This is the lowest
    level component which defines the shader states for mesh rendering.

    (C) 2004 RadonLabs GmbH
*/
#include "renderpath/nrpshader.h"
#include "variable/nvariablecontext.h"
#include "variable/nvariableserver.h"
#include "kernel/nprofiler.h"

class nRenderPath2;
class nRpSection;
class nRpPass;
class nRpPhase;

//------------------------------------------------------------------------------
class nRpSequence
{
public:
    /// constructor
    nRpSequence();
    /// destructor
    ~nRpSequence();
    /// set the renderpath
    void SetRenderPath(nRenderPath2* rp);
    /// get the renderpath
    nRenderPath2* GetRenderPath();
    /// set shader alias
    void SetShaderAlias(const nString& p);
    /// get shader alias
    const nString& GetShaderAlias() const;
    /// set optional technique
    void SetTechnique(const nString& tec);
    /// get optional technique
    const nString& GetTechnique() const;
    /// get shader index of embedded shader
    int GetShaderBucketIndex() const;
    /// enable/disable alpha blending in first light pass
    void SetFirstLightAlphaEnabled(bool b);
    /// get first light alpha state
    bool GetFirstLightAlphaEnabled() const;
    /// set shader-updates enabled flag
    void SetShaderUpdatesEnabled(bool b);
    /// get shader-update enabled flag
    bool GetShaderUpdatesEnabled() const;
    /// set mvp-only hint (only update ModelViewProjection in shaders)
    void SetMvpOnlyHint(bool b);
    /// get mvp-only hint
    bool GetMvpOnlyHint() const;
    /// begin rendering the sequence  shader
    int Begin();
    /// begin rendering a shader pass
    void BeginPass(int pass);
    /// end rendering the current shader pass
    void EndPass();
    /// end rendering the sequence shader
    void End();
    /// add constant shader parameter value
    void AddConstantShaderParam(nShaderState::Param p, const nShaderArg& arg);
    /// add variable shader parameter value
    void AddVariableShaderParam(const nString& varName, nShaderState::Param p, const nShaderArg& arg);

#if __NEBULA_STATS__
    /// set the section
    void SetSection(nRpSection* rp);
    /// get the section
    nRpSection* GetSection() const;
    /// set the pass
    void SetPass(nRpPass* p);
    /// get the pass
    nRpPass* GetPass() const;
    /// set the phase
    void SetPhase(nRpPhase* ph);
    /// get the phase
    nRpPhase* GetPhase() const;
#endif

private:
    friend class nRpPhase;

    /// validate the sequence object
    void Validate();
    /// update the variable shader parameters
    void UpdateVariableShaderParams();

    nRenderPath2* renderPath;
    nString shaderAlias;
    nShaderParams shaderParams;
    nVariableContext varContext;
    nString technique;
    int rpShaderIndex;
    bool firstLightAlphaEnabled;
    bool shaderUpdatesEnabled;
    bool mvpOnly;                   // true if sequence shader only required the ModelViewProjection matrix

#if __NEBULA_STATS__
//    nProfiler prof;
    nRpSection* section;
    nRpPass* pass;
    nRpPhase* phase;
#endif
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpSequence::SetMvpOnlyHint(bool b)
{
    this->mvpOnly = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nRpSequence::GetMvpOnlyHint() const
{
    return this->mvpOnly;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpSequence::SetShaderUpdatesEnabled(bool b)
{
    this->shaderUpdatesEnabled = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nRpSequence::GetShaderUpdatesEnabled() const
{
    return this->shaderUpdatesEnabled;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpSequence::SetShaderAlias(const nString& p)
{
    this->shaderAlias = p;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nString&
nRpSequence::GetShaderAlias() const
{
    return this->shaderAlias;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpSequence::SetRenderPath(nRenderPath2* rp)
{
    n_assert(rp);
    this->renderPath = rp;
}

//------------------------------------------------------------------------------
/**
*/
inline
nRenderPath2*
nRpSequence::GetRenderPath()
{
    return this->renderPath;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpSequence::SetTechnique(const nString& tec)
{
    this->technique = tec;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nString&
nRpSequence::GetTechnique() const
{
    return this->technique;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpSequence::AddConstantShaderParam(nShaderState::Param p, const nShaderArg& arg)
{
    // add the shader param to the parameter block
    this->shaderParams.SetArg(p, arg);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpSequence::AddVariableShaderParam(const nString& varName, nShaderState::Param p, const nShaderArg& arg)
{
    // add shader param to parameter block
    this->shaderParams.SetArg(p, arg);

    // add a variable name to shader state mapping to the variable context
    nVariable::Handle h = nVariableServer::Instance()->GetVariableHandleByName(varName.Get());
    nVariable var(h, int(p));
    this->varContext.AddVariable(var);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpSequence::SetFirstLightAlphaEnabled(bool b)
{
    this->firstLightAlphaEnabled = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nRpSequence::GetFirstLightAlphaEnabled() const
{
    return this->firstLightAlphaEnabled;
}

#if __NEBULA_STATS__
//------------------------------------------------------------------------------
/**
*/
inline
void
nRpSequence::SetSection(nRpSection* s)
{
    n_assert(s);
    this->section = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
nRpSection*
nRpSequence::GetSection() const
{
    return this->section;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpSequence::SetPass(nRpPass* p)
{
    n_assert(p);
    this->pass = p;
}

//------------------------------------------------------------------------------
/**
*/
inline
nRpPass*
nRpSequence::GetPass() const
{
    return this->pass;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpSequence::SetPhase(nRpPhase* ph)
{
    n_assert(ph);
    this->phase = ph;
}

//------------------------------------------------------------------------------
/**
*/
inline
nRpPhase*
nRpSequence::GetPhase() const
{
    return this->phase;
}
#endif

//------------------------------------------------------------------------------
#endif
