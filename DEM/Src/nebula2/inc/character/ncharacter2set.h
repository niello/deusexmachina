#ifndef N_CHARACTER2SET_H
#define N_CHARACTER2SET_H
//------------------------------------------------------------------------------
/**
    @class nCharacter2Set
    @ingroup Character

    (C) 2005 RadonLabs GmbH
*/
#include <Core/RefCounted.h>
#include "util/nstring.h"
#include "kernel/nRefCounted.h"

//------------------------------------------------------------------------------
class nCharacter2Set: public nRefCounted //Core::CRefCounted
{
public:
    /// constructor
    nCharacter2Set();
    /// destructor
    virtual ~nCharacter2Set();
    /// set fade-in time when being applied
    void SetFadeInTime(float time);
    /// get fade-in time when being applied
    float GetFadeInTime() const;
    /// add a clip with name and weight to the animation (for mixing)
    void AddClip(const nString& clipName, float clipWeight);
    /// remove a clip by name
    void RemoveClip(const nString& clipName);
    /// set the weight of a clip
    void SetClipWeightAt(int index, float clipWeight);
    /// get clip index by name
    int GetClipIndexByName(const nString& clipName) const;
    /// get the name of a clip
    const nString& GetClipNameAt(int index) const;
    /// get the weight of a clip
    float GetClipWeightAt(int index) const;
    /// get number of clips
    int GetNumClips() const;
    /// remove all clips
    void ClearClips();
    /// has been changed
    bool IsDirty() const;
    /// set dirty flag
    void SetDirty(bool b);

protected:
    bool dirty;
    float fadeInTime;
    nArray<nString> clipNames;
    nArray<float> clipWeights;
};

//------------------------------------------------------------------------------
/**
*/
inline
nCharacter2Set::nCharacter2Set():
     dirty(false),
     fadeInTime(1.0f)
{
    /// empty
}

//------------------------------------------------------------------------------
/**
*/
inline
nCharacter2Set::~nCharacter2Set()
{
    /// empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCharacter2Set::SetFadeInTime(float time)
{
    this->fadeInTime = time;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nCharacter2Set::GetFadeInTime() const
{
    return this->fadeInTime;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCharacter2Set::AddClip(const nString& clipName, float clipWeight)
{
    this->clipNames.PushBack(clipName);
    this->clipWeights.PushBack(clipWeight);
    this->dirty = true;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCharacter2Set::RemoveClip(const nString& clipName)
{
    int index = this->GetClipIndexByName(clipName);
    n_assert(index != -1);
    this->clipNames.Erase(index);
    this->clipWeights.Erase(index);
    this->dirty = true;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCharacter2Set::SetClipWeightAt(int index, float clipWeight)
{
    this->clipWeights[index] = clipWeight;
    this->dirty = true;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nCharacter2Set::IsDirty() const
{
    return this->dirty;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCharacter2Set::SetDirty(bool b)
{
    this->dirty = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nCharacter2Set::GetNumClips() const
{
    return this->clipNames.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nCharacter2Set::GetClipIndexByName(const nString& name) const
{
    int i;
    for (i = 0; i < this->clipNames.Size(); i++)
    {
        if (this->clipNames[i] == name)
        {
            return i;
        }
    }

    return -1;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nString&
nCharacter2Set::GetClipNameAt(int index) const
{
    return this->clipNames[index];
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nCharacter2Set::GetClipWeightAt(int index) const
{
    return this->clipWeights[index];
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCharacter2Set::ClearClips()
{
    this->clipWeights.Clear();
    this->clipNames.Clear();
    this->dirty = true;
}

//------------------------------------------------------------------------------

#endif
