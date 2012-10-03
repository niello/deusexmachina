#ifndef N_ANIMCLIP_H
#define N_ANIMCLIP_H
//------------------------------------------------------------------------------
/**
    @class nAnimClip
    @ingroup Anim2

    @brief An animation clip bundles several animation curves into an unit
    and associates them with a weight value.

    (C) 2003 RadonLabs GmbH
*/
#include "anim2/nanimation.h"
#include "variable/nvariable.h"
#include "anim2/nanimeventtrack.h"

//------------------------------------------------------------------------------
class nAnimClip
{
public:
    /// default constructor
    nAnimClip();
    /// constructor
    nAnimClip(const nString& clipName, int animGroupIndex, int numCurves);
    /// get the name of the clip
    const nString& GetClipName() const;
    /// get animation group index
    int GetAnimGroupIndex() const;
    /// get number of curves in the clip's animation group
    int GetNumCurves() const;
    /// set number of animation event tracks
    void SetNumAnimEventTracks(int num);
    /// get number of animation event tracks
    int GetNumAnimEventTracks() const;
    /// get to animation event track
    const nAnimEventTrack& GetAnimEventTrackAt(int index) const;
    /// read/write access to animation event tracks array
    nFixedArray<nAnimEventTrack>& GetAnimEventTracks();

private:
    nString clipName;
    int animGroupIndex;
    int numCurves;
    nFixedArray<nAnimEventTrack> animEventTracks;
};

//------------------------------------------------------------------------------
/**
*/
inline
nAnimClip::nAnimClip() :
    animGroupIndex(0),
    numCurves(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
nAnimClip::nAnimClip(const nString& name, int animGroupIndex, int numCurves) :
   clipName(name),
   animGroupIndex(animGroupIndex),
   numCurves(numCurves)
{
}

//------------------------------------------------------------------------------
/**
*/
inline
const nString&
nAnimClip::GetClipName() const
{
    return this->clipName;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nAnimClip::GetAnimGroupIndex() const
{
    return this->animGroupIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nAnimClip::GetNumCurves() const
{
    return this->numCurves;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimClip::SetNumAnimEventTracks(int num)
{
    this->animEventTracks.SetSize(num);
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nAnimClip::GetNumAnimEventTracks() const
{
    return this->animEventTracks.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline
const nAnimEventTrack&
nAnimClip::GetAnimEventTrackAt(int index) const
{
    return this->animEventTracks[index];
}

//------------------------------------------------------------------------------
/**
*/
inline
nFixedArray<nAnimEventTrack>&
nAnimClip::GetAnimEventTracks()
{
    return this->animEventTracks;
}

//------------------------------------------------------------------------------
#endif
