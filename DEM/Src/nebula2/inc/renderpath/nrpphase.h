#ifndef N_RPPHASE_H
#define N_RPPHASE_H
//------------------------------------------------------------------------------
/**
    @class nRpPhase
    @ingroup RenderPath
    @brief A phase object inside a render path pass encapsulates sequence
    shaders and sets common render state for sequence shaders.

    (C) 2004 RadonLabs GmbH
*/
#include "util/nstring.h"
#include "renderpath/nrpsequence.h"
#include "renderpath/nrpshader.h"
#include "kernel/nprofiler.h"

namespace Render
{
	class CPass;
}

class nRpPhase
{
public:
    /// sorting orders
    enum SortingOrder
    {
        None,
        FrontToBack,
        BackToFront,
    };

    // lighting modes
    enum LightMode
    {
        Off,
        Shader,
    };

    /// constructor
    nRpPhase();
    /// set the renderpath
    void SetRenderPath(CFrameShader* rp);
    /// get the renderpath
    CFrameShader* GetRenderPath() const;
    /// set phase name
    void SetName(const nString& n);
    /// get phase name
    const nString& GetName() const;
    /// set phase shader alias
    void SetShaderAlias(const nString& p);
    /// get phase shader alias
    const nString& GetShaderAlias() const;
    /// set optional technique
    void SetTechnique(const nString& n);
    /// get optional shader technique
    const nString& GetTechnique() const;
    /// set sorting order
    void SetSortingOrder(SortingOrder o);
    /// get sorting order
    SortingOrder GetSortingOrder() const;
    /// set lighting mode
    void SetLightMode(LightMode m);
    /// get lighting mode
    LightMode GetLightMode() const;
    /// add a sequence object
    void AddSequence(const nRpSequence& seq);
    /// get array of sequences
    const nArray<nRpSequence>& GetSequences() const;
    /// begin rendering the phase
    int Begin();
    /// get sequence by index
    nRpSequence& GetSequence(int i) const;
    /// finish rendering the phase
    void End();
    /// convert string to sorting order
    static SortingOrder StringToSortingOrder(const char* str);
    /// convert string to lighting mode
    static LightMode StringToLightMode(const char* str);

    /// validate the pass object
    void Validate();

private:

    CFrameShader* pFrameShader;
    bool inBegin;
    nString name;
    nString shaderAlias;
    nString technique;
    int rpShaderIndex;
    SortingOrder sortingOrder;
    LightMode lightMode;
    nArray<nRpSequence> sequences;
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPhase::SetTechnique(const nString& t)
{
    this->technique = t;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nString&
nRpPhase::GetTechnique() const
{
    return this->technique;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPhase::SetName(const nString& n)
{
    this->name = n;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nString&
nRpPhase::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPhase::SetShaderAlias(const nString& p)
{
    this->shaderAlias = p;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPhase::SetRenderPath(CFrameShader* rp)
{
    n_assert(rp);
    this->pFrameShader = rp;
}

//------------------------------------------------------------------------------
/**
*/
inline
CFrameShader*
nRpPhase::GetRenderPath() const
{
    return this->pFrameShader;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nString&
nRpPhase::GetShaderAlias() const
{
    return this->shaderAlias;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPhase::SetSortingOrder(SortingOrder o)
{
    this->sortingOrder = o;
}

//------------------------------------------------------------------------------
/**
*/
inline
nRpPhase::SortingOrder
nRpPhase::GetSortingOrder() const
{
    return this->sortingOrder;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPhase::SetLightMode(LightMode m)
{
    this->lightMode = m;
}

//------------------------------------------------------------------------------
/**
*/
inline
nRpPhase::LightMode
nRpPhase::GetLightMode() const
{
    return this->lightMode;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpPhase::AddSequence(const nRpSequence& seq)
{
    this->sequences.Append(seq);
}

//------------------------------------------------------------------------------
/**
*/
inline
const nArray<nRpSequence>&
nRpPhase::GetSequences() const
{
    return this->sequences;
}

//------------------------------------------------------------------------------
/**
*/
inline
nRpSequence&
nRpPhase::GetSequence(int i) const
{
    return this->sequences[i];
}

//------------------------------------------------------------------------------
/**
*/
inline
nRpPhase::SortingOrder
nRpPhase::StringToSortingOrder(const char* str)
{
    n_assert(str);
    if (0 == strcmp("None", str)) return None;
    else if (0 == strcmp("FrontToBack", str)) return FrontToBack;
    else if (0 == strcmp("BackToFront", str)) return BackToFront;
    else
    {
        n_error("nRpPhase::StringToSortingOrder(): invalid string '%s'!", str);
        return None;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
nRpPhase::LightMode
nRpPhase::StringToLightMode(const char* str)
{
    n_assert(str);
    if (0 == strcmp("Off", str)) return Off;
    else if (0 == strcmp("Shader", str)) return Shader;
    else
    {
        n_error("nRpPhase::StringToLightMode(): invalid string '%s'!", str);
        return Off;
    }
}


//------------------------------------------------------------------------------
#endif
