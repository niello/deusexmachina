#ifndef N_ANIMSTATEINFO_H
#define N_ANIMSTATEINFO_H

#include "kernel/ntypes.h"
#include "anim2/nanimclip.h"

/**
    @class nAnimStateInfo
    @ingroup Anim2

    (C) 2006 RadonLabs GmbH

    @brief An animation state info contains any number of nAnimClip objects of
    identical size (number of animation curves contained in the clip) for
    sampling a weight-blended result from them. Each animation clip is associated
    with a weight value between 0 and 1 which defines how much that animation
    clip influences the resulting animation.
*/
class nAnimStateInfo
{
public:
    /// constructor
    nAnimStateInfo();
    /// set the fade in time
    void SetFadeInTime(float t);
    /// get the fade in time
    float GetFadeInTime() const;
    /// set state started time
    void SetStateStarted(float t);
    /// get state started time
    float GetStateStarted() const;
    /// set state time offset
    void SetStateOffset(float t);
    /// get state time offset
    float GetStateOffset() const;
    /// is valid?
    bool IsValid() const;

    /// begin defining animation clips
    void BeginClips(int num);
    /// set an animation clip
    void SetClip(int index, const nAnimClip& clip, float weight);
    /// finish defining animation clips
    void EndClips();
    /// get number of animation clips
    int GetNumClips() const;
    /// get animation clip at index
    nAnimClip& GetClipAt(int index) const;
    /// get weight of clip at index
    float GetClipWeightAt(int index) const;

    nArray<nAnimClip> clipArray;
    nArray<float> clipWeights;
    float fadeInTime;
    float stateStarted;
    float stateOffset;
};

//------------------------------------------------------------------------------
/**
*/
inline
nAnimStateInfo::nAnimStateInfo() :
    clipArray(0, 0),
    clipWeights(0, 0),
    fadeInTime(0.0f),
    stateStarted(0.0f),
    stateOffset(0.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimStateInfo::SetFadeInTime(float t)
{
    this->fadeInTime = t;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nAnimStateInfo::GetFadeInTime() const
{
    return this->fadeInTime;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimStateInfo::SetStateStarted(float t)
{
    this->stateStarted = t;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nAnimStateInfo::GetStateStarted() const
{
    return this->stateStarted;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimStateInfo::SetStateOffset(float t)
{
    this->stateOffset= t;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nAnimStateInfo::GetStateOffset() const
{
    return this->stateOffset;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nAnimStateInfo::IsValid() const
{
    return (!this->clipArray.Empty());
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimStateInfo::BeginClips(int num)
{
    this->clipArray.SetFixedSize(num);
    this->clipWeights.SetFixedSize(num);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimStateInfo::SetClip(int index, const nAnimClip& clip, float weight)
{
    n_assert(index >= 0);
    n_assert(weight >= 0.0f && weight <= 1.0f);

    // ensure identical number of curves in clips
    if (this->clipArray[0].GetNumCurves() > 0)
    {
        if (this->clipArray[0].GetNumCurves() != clip.GetNumCurves())
        {
            n_error("Only Clips with identical number of curves can be active at one time.");
        }
    }

    this->clipArray[index] = clip;
    this->clipWeights[index] = weight;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimStateInfo::EndClips()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nAnimStateInfo::GetNumClips() const
{
    return this->clipArray.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline
nAnimClip&
nAnimStateInfo::GetClipAt(int index) const
{
    return this->clipArray[index];
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nAnimStateInfo::GetClipWeightAt(int index) const
{
    return this->clipWeights[index];
}

//------------------------------------------------------------------------------
#endif
