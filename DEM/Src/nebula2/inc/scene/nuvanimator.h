#ifndef N_UVANIMATOR_H
#define N_UVANIMATOR_H
//------------------------------------------------------------------------------
/**
    @class nUvAnimator
    @ingroup Scene

    @brief Animates the UV transform parameters on an nAbstractShaderNode.

    See also @ref N2ScriptInterface_nuvanimator

    -01-Nov-06  kims Changed vector2 to vector3 type on AddEulerKey() and 
                     GetEulerKeyAt() functions. The changes were done for uv animation.

    (C) 2004 RadonLabs GmbH
*/
#include "scene/nanimator.h"
#include "util/nanimkeyarray.h"
#include "gfx2/ngfxserver2.h"

//------------------------------------------------------------------------------
class nUvAnimator : public nAnimator
{
public:
    /// constructor
    nUvAnimator();
    /// destructor
    virtual ~nUvAnimator();
    /// save object to persistent stream
    //virtual bool SaveCmds(nPersistServer* ps);

    /// return the type of this animator object (SHADER)
    virtual Type GetAnimatorType() const;
    /// called by scene node objects which wish to be animated by this object
    virtual void Animate(nSceneNode* sceneNode, nRenderContext* renderContext);
    /// add a position key
    void AddPosKey(uint texLayer, float time, const vector2& key);
    /// add a euler angle key
    void AddEulerKey(uint texLayer, float time, const vector3& key);
    /// add a scale key
    void AddScaleKey(uint texLayer, float time, const vector2& key);
    /// get number of position keys
    int GetNumPosKeys(uint texLayer) const;
    /// get position key at index
    void GetPosKeyAt(uint texLayer, uint keyIndex, float& time, vector2& key) const;
    /// get number of euler angle keys
    int GetNumEulerKeys(uint texLayer) const;
    /// get euler key at index
    void GetEulerKeyAt(uint texLayer, uint keyIndex, float& time, vector3& key) const;
    /// get number of scale keys
    int GetNumScaleKeys(uint texLayer) const;
    /// get scale key at index
    void GetScaleKeyAt(uint texLayer, uint keyIndex, float& time, vector2& key) const;

private:
    nAnimKeyArray<nAnimKey<vector2> > posArray[nGfxServer2::MaxTextureStages];
    nAnimKeyArray<nAnimKey<vector3> > eulerArray[nGfxServer2::MaxTextureStages];
    nAnimKeyArray<nAnimKey<vector2> > scaleArray[nGfxServer2::MaxTextureStages];
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nUvAnimator::AddPosKey(uint texLayer, float time, const vector2& key)
{
    n_assert(texLayer < nGfxServer2::MaxTextureStages);
    nAnimKey<vector2> newKey(time, key);
    this->posArray[texLayer].Append(newKey);
}

//------------------------------------------------------------------------------
/**
    -01-Nov-06  kims Changed to have vector3 in-arg for uv animation.
*/
inline
void
nUvAnimator::AddEulerKey(uint texLayer, float time, const vector3& key)
{
    n_assert(texLayer < nGfxServer2::MaxTextureStages);
    nAnimKey<vector3> newKey(time, key);
    this->eulerArray[texLayer].Append(newKey);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nUvAnimator::AddScaleKey(uint texLayer, float time, const vector2& key)
{
    n_assert(texLayer < nGfxServer2::MaxTextureStages);
    nAnimKey<vector2> newKey(time, key);
    this->scaleArray[texLayer].Append(newKey);
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nUvAnimator::GetNumPosKeys(uint texLayer) const
{
    n_assert(texLayer < nGfxServer2::MaxTextureStages);
    return this->posArray[texLayer].Size();
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nUvAnimator::GetNumEulerKeys(uint texLayer) const
{
    n_assert(texLayer < nGfxServer2::MaxTextureStages);
    return this->eulerArray[texLayer].Size();
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nUvAnimator::GetNumScaleKeys(uint texLayer) const
{
    n_assert(texLayer < nGfxServer2::MaxTextureStages);
    return this->scaleArray[texLayer].Size();
}

//------------------------------------------------------------------------------
/**
    Obtain a position key by its index.

    @param  texLayer    [in]    texture layer index
    @param  keyIndex    [in]    index of key to get
    @param  time        [out]   the time stamp of the key
    @param  key         [out]   the value of the key
*/
inline
void
nUvAnimator::GetPosKeyAt(uint texLayer, uint keyIndex, float& time, vector2& key) const
{
    n_assert(texLayer < nGfxServer2::MaxTextureStages);
    const nAnimKey<vector2>& k = this->posArray[texLayer][keyIndex];
    time = k.GetTime();
    key = k.GetValue();
}

//------------------------------------------------------------------------------
/**
    Obtain a euler key by its index.

    -01-Nov-06  kims Changed to have vector3 in-arg for uv animation.

    @param  texLayer    [in]    texture layer index
    @param  keyIndex    [in]    index of key to get
    @param  time        [out]   the time stamp of the key
    @param  key         [out]   the value of the key
*/
inline
void
nUvAnimator::GetEulerKeyAt(uint texLayer, uint keyIndex, float& time, vector3& key) const
{
    n_assert(texLayer < nGfxServer2::MaxTextureStages);
    const nAnimKey<vector3>& k = this->eulerArray[texLayer][keyIndex];
    time = k.GetTime();
    key = k.GetValue();
}

//------------------------------------------------------------------------------
/**
    Obtain a scale key by its index.

    @param  texLayer    [in]    texture layer index
    @param  keyIndex    [in]    index of key to get
    @param  time        [out]   the time stamp of the key
    @param  key         [out]   the value of the key
*/
inline
void
nUvAnimator::GetScaleKeyAt(uint texLayer, uint keyIndex, float& time, vector2& key) const
{
    n_assert(texLayer < nGfxServer2::MaxTextureStages);
    const nAnimKey<vector2>& k = this->scaleArray[texLayer][keyIndex];
    time = k.GetTime();
    key = k.GetValue();
}
//------------------------------------------------------------------------------
#endif

