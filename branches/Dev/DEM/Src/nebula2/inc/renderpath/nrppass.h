#ifndef N_RPPASS_H
#define N_RPPASS_H
//------------------------------------------------------------------------------
/**
    @class nRpPass
    @ingroup RenderPath
    @brief Represents a pass in a render target.

    (C) 2004 RadonLabs GmbH
*/
#include "renderpath/nrpphase.h"
#include "renderpath/nrpshader.h"
#include "variable/nvariablecontext.h"
#include "variable/nvariableserver.h"
#include "gfx2/nmesh2.h"
#include "kernel/nprofiler.h"

class nRenderPath2;
class nRpSection;

//------------------------------------------------------------------------------
class nRpPass
{
public:
    /// shadow drawing techniques
    enum ShadowTechnique
    {
        NoShadows,      // don't draw shadows
        Simple,         // draw simple DX7 style shadows
        MultiLight,     // draw multilight shadows
    };

    /// constructor
    nRpPass();
    /// destructor
    ~nRpPass();
    /// assignment operator
    void operator=(const nRpPass& rhs);
    /// set the renderpath
    void SetRenderPath(nRenderPath2* rp);
    /// get the renderpath
    nRenderPath2* GetRenderPath() const;
    /// set pass name
    void SetName(const nString& n);
    /// get pass name
    const nString& GetName() const;
    /// set the pass shader alias
    void SetShaderAlias(const nString& n);
    /// get the pass shader alias
    const nString& GetShaderAlias() const;
    /// get the shader object assigned to this pass (can be 0)
    nShader2* GetShader() const;
    /// set optional technique
    void SetTechnique(const nString& n);
    /// get optional technique
    const nString& GetTechnique() const;
    /// set the render target's name (0 for default render target)
    void SetRenderTargetName(int index, const nString& n);
    /// get the render target's name
    const nString& GetRenderTargetName(const int index) const;
    /// set clear flags (nGfxServer2::BufferType)
    void SetClearFlags(int f);
    /// get clear flags
    int GetClearFlags() const;
    /// set clear color
    void SetClearColor(const vector4& c);
    /// get clear color
    const vector4& GetClearColor() const;
    /// set clear depth
    void SetClearDepth(float d);
    /// get clear depth
    float GetClearDepth() const;
    /// set clear stencil value
    void SetClearStencil(int v);
    /// get clear stencil value
    int GetClearStencil() const;
    /// set the draw full-screen quad flag
    void SetDrawFullscreenQuad(bool b);
    /// get the draw full-screen quad flag
    bool GetDrawFullscreenQuad() const;
    /// set the "draw shadow volumes" technique
    void SetDrawShadows(ShadowTechnique t);
    /// get the "draw shadow volumes" technique
    ShadowTechnique GetDrawShadows() const;
    /// enable/disable statistics counter in this pass
    void SetStatsEnabled(bool b);
    /// get statistics counter flag for this pass
    bool GetStatsEnabled() const;
    /// set the occlusion query technique
    void SetOcclusionQuery(bool b);
    /// get the occlusion query technique
    bool GetOcclusionQuery() const;
    /// set the "shadow enabled condition" flag
    void SetShadowEnabledCondition(bool b);
    /// get the "shadow enabled condition" flag
    bool GetShadowEnabledCondition() const;
    /// set the "draw gui" flag
    void SetDrawGui(bool b);
    /// get the "draw gui" flag
    bool GetDrawGui() const;
    /// add constant shader parameter value
    void AddConstantShaderParam(nShaderState::Param p, const nShaderArg& arg);
    /// add variable shader parameter value
    void AddVariableShaderParam(const nString& varName, nShaderState::Param p, const nShaderArg& arg);
    /// access to shader parameter block
    const nShaderParams& GetShaderParams() const;
    /// add optional phase objects
    void AddPhase(const nRpPhase& phase);
    /// get array of phases
    const nArray<nRpPhase>& GetPhases() const;
    /// begin rendering the pass
    int Begin();
    /// get phase object at index
    nRpPhase& GetPhase(int i) const;
    /// finish rendering the pass
    void End();
    /// convert shadow technique string to enum
    static ShadowTechnique StringToShadowTechnique(const char* str);
    /// draw a full-screen quad
    void DrawFullScreenQuad();

#if __NEBULA_STATS__
    /// set the section
    void SetSection(nRpSection* s);
    /// get the section
    nRpSection* GetSection() const;
#endif

private:
    friend class nRpSection;

    /// validate the pass object
    void Validate();
    /// update the variable shader parameters
    void UpdateVariableShaderParams();
    /// update quad mesh coordinates
    void UpdateMeshCoords();

    struct ShaderParam
    {
        nShaderState::Type type;
        nString stateName;
        nString value;
    };

    nRenderPath2* renderPath;
    nArray<ShaderParam> constShaderParams;
    nArray<ShaderParam> varShaderParams;

    bool inBegin;
    nString name;
    nString shaderAlias;
    nString technique;
    nFourCC shaderFourCC;
    nFixedArray<nString> renderTargetNames;

    int rpShaderIndex;
    nShaderParams shaderParams;
    nRef<nMesh2> refQuadMesh;

    nArray<nRpPhase> phases;
    nVariableContext varContext;

    int clearFlags;
    vector4 clearColor;
    float clearDepth;
    int clearStencil;
    ShadowTechnique shadowTechnique;

    bool drawFullscreenQuad;        // true if pass should render a full-screen quad
    bool drawGui;                   // true if this pass should render the gui
    bool shadowEnabledCondition;
    bool occlusionQuery;            // special flag for occlusion query
    bool statsEnabled;

    #if __NEBULA_STATS__
    nProfiler prof;
    nRpSection* section;
    #endif
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPass::operator=(const nRpPass& rhs)
{
    this->renderPath                = rhs.renderPath;
    this->constShaderParams         = rhs.constShaderParams;
    this->varShaderParams           = rhs.varShaderParams;
    this->inBegin                   = rhs.inBegin;
    this->name                      = rhs.name;
    this->shaderAlias               = rhs.shaderAlias;
    this->technique                 = rhs.technique;
    this->renderTargetNames         = rhs.renderTargetNames;
    this->shaderParams              = rhs.shaderParams;
    this->rpShaderIndex             = rhs.rpShaderIndex;
    this->refQuadMesh               = rhs.refQuadMesh;
    this->phases                    = rhs.phases;
    this->varContext                = rhs.varContext;
    this->clearFlags                = rhs.clearFlags;
    this->clearColor                = rhs.clearColor;
    this->clearDepth                = rhs.clearDepth;
    this->clearStencil              = rhs.clearStencil;
    this->shadowTechnique           = rhs.shadowTechnique;
    this->occlusionQuery            = rhs.occlusionQuery;
    this->statsEnabled              = rhs.statsEnabled;
    this->drawFullscreenQuad        = rhs.drawFullscreenQuad;
    this->drawGui                   = rhs.drawGui;
    this->shadowEnabledCondition    = rhs.shadowEnabledCondition;
    if (this->refQuadMesh.isvalid())
    {
        this->refQuadMesh->AddRef();
    }
    #if __NEBULA_STATS__
    this->section                   = rhs.section;
    #endif
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPass::SetStatsEnabled(bool b)
{
    this->statsEnabled = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nRpPass::GetStatsEnabled() const
{
    return this->statsEnabled;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPass::SetOcclusionQuery(bool b)
{
    this->occlusionQuery = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nRpPass::GetOcclusionQuery() const
{
    return this->occlusionQuery;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPass::SetRenderPath(nRenderPath2* rp)
{
    n_assert(rp);
    this->renderPath = rp;
}

//------------------------------------------------------------------------------
/**
*/
inline
nRenderPath2*
nRpPass::GetRenderPath() const
{
    return this->renderPath;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPass::SetShaderAlias(const nString& s)
{
    this->shaderAlias = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nString&
nRpPass::GetShaderAlias() const
{
    return this->shaderAlias;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPass::SetTechnique(const nString& t)
{
    this->technique = t;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nString&
nRpPass::GetTechnique() const
{
    return this->technique;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPass::SetDrawFullscreenQuad(bool b)
{
    this->drawFullscreenQuad = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nRpPass::GetDrawFullscreenQuad() const
{
    return this->drawFullscreenQuad;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPass::SetDrawGui(bool b)
{
    this->drawGui = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nRpPass::GetDrawGui() const
{
    return this->drawGui;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPass::SetClearFlags(int f)
{
    this->clearFlags = f;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nRpPass::GetClearFlags() const
{
    return this->clearFlags;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPass::SetClearColor(const vector4& c)
{
    this->clearColor = c;
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector4&
nRpPass::GetClearColor() const
{
    return this->clearColor;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPass::SetClearDepth(float d)
{
    this->clearDepth = d;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nRpPass::GetClearDepth() const
{
    return this->clearDepth;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPass::SetClearStencil(int v)
{
    this->clearStencil = v;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nRpPass::GetClearStencil() const
{
    return this->clearStencil;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPass::SetName(const nString& n)
{
    this->name = n;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nString&
nRpPass::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPass::SetRenderTargetName(int index, const nString& n)
{
    this->renderTargetNames[index] = n;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nString&
nRpPass::GetRenderTargetName(const int index) const
{
    return this->renderTargetNames[index];
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPass::AddConstantShaderParam(nShaderState::Param p, const nShaderArg& arg)
{
    // add the shader param to the parameter block
    this->shaderParams.SetArg(p, arg);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPass::AddVariableShaderParam(const nString& varName, nShaderState::Param p, const nShaderArg& arg)
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
const nShaderParams&
nRpPass::GetShaderParams() const
{
    return this->shaderParams;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPass::AddPhase(const nRpPhase& p)
{
    this->phases.Append(p);
}

//------------------------------------------------------------------------------
/**
*/
inline
const nArray<nRpPhase>&
nRpPass::GetPhases() const
{
    return this->phases;
}

//------------------------------------------------------------------------------
/**
*/
inline
nRpPhase&
nRpPass::GetPhase(int i) const
{
    return this->phases[i];
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPass::SetShadowEnabledCondition(bool b)
{
    this->shadowEnabledCondition = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nRpPass::GetShadowEnabledCondition() const
{
    return this->shadowEnabledCondition;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPass::SetDrawShadows(ShadowTechnique t)
{
    this->shadowTechnique = t;
}

//------------------------------------------------------------------------------
/**
*/
inline
nRpPass::ShadowTechnique
nRpPass::GetDrawShadows() const
{
    return this->shadowTechnique;
}

//------------------------------------------------------------------------------
/**
*/
inline
nRpPass::ShadowTechnique
nRpPass::StringToShadowTechnique(const char* str)
{
    n_assert(str);
    if (strcmp(str, "NoShadows") == 0)      return NoShadows;
    if (strcmp(str, "Simple") == 0)         return Simple;
    if (strcmp(str, "MultiLight") == 0)     return MultiLight;
    n_error("nRpPass::StringToShadowTechnique: Invalid string '%s'!", str);
    return NoShadows;
}

#if __NEBULA_STATS__
//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPass::SetSection(nRpSection* s)
{
    n_assert(s);
    this->section = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
nRpSection*
nRpPass::GetSection() const
{
    return this->section;
}
#endif

//------------------------------------------------------------------------------
#endif


