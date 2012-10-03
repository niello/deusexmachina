#ifndef N_RPSECTION_H
#define N_RPSECTION_H
//------------------------------------------------------------------------------
/**
    @class nRpSection
    @ingroup RenderPath
    @brief A render path can have several sections, each describing how a
    complete scene is rendered into a render target. Sections are used by
    scene camera nodes to render their view before that actual default
    section is rendered which may need to access render targets prepared
    by the cameras.

    (C) 2005 Radon Labs GmbH
*/
#include "renderpath/nrppass.h"

class nRenderPath2;

//------------------------------------------------------------------------------
class nRpSection
{
public:
    /// constructor
    nRpSection();
    /// destructor
    ~nRpSection();
    /// set the render path
    void SetRenderPath(nRenderPath2* rp);
    /// get the render path
    nRenderPath2* GetRenderPath() const;
    /// set the section name
    void SetName(const nString& n);
    /// get the section name
    const nString& GetName() const;
    /// add pass object
    void AddPass(const nRpPass& pass);
    /// get array of passes
    const nArray<nRpPass>& GetPasses() const;
    /// begin rendering through the section
    int Begin();
    /// get pass at index
    nRpPass& GetPass(int i) const;
    /// finish rendering the section
    void End();
    /// return true if inside begin/end
    bool InBegin() const;

private:
    /// validate the section object
    void Validate();

    friend class nRenderPath2;

    nRenderPath2* renderPath;
    nString name;
    bool inBegin;
    nArray<nRpPass> passes;

    #if __NEBULA_STATS__
    nProfiler prof;
    #endif
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpSection::SetName(const nString& n)
{
    this->name = n;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nString&
nRpSection::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nRpSection::InBegin() const
{
    return this->inBegin;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpSection::AddPass(const nRpPass& p)
{
    this->passes.Append(p);
}

//------------------------------------------------------------------------------
/**
*/
inline
nRpPass&
nRpSection::GetPass(int index) const
{
    return this->passes[index];
}

//------------------------------------------------------------------------------
/**
*/
inline
const nArray<nRpPass>&
nRpSection::GetPasses() const
{
    return this->passes;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpSection::SetRenderPath(nRenderPath2* rp)
{
    this->renderPath = rp;
}

//------------------------------------------------------------------------------
/**
*/
inline
nRenderPath2*
nRpSection::GetRenderPath() const
{
    return this->renderPath;
}

//------------------------------------------------------------------------------
#endif
