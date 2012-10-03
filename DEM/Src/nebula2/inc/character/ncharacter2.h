#ifndef N_CHARACTER2_H
#define N_CHARACTER2_H
//------------------------------------------------------------------------------
/**
    @class nCharacter2
    @ingroup Character

    @brief Holds all the data necessary to animate an character in one place.

    (C) 2003 RadonLabs GmbH
*/
#include <Core/RefCounted.h>
#include "character/ncharskeleton.h"
#include "anim2/nanimstateinfo.h"
#include "kernel/nRefCounted.h"

class nVariableContext;
class nSkinAnimator;
class nAnimEventHandler;

//------------------------------------------------------------------------------
class nCharacter2: public nRefCounted //Core::CRefCounted
{
public:
    /// constructor
    nCharacter2();
    /// copy constructor
    nCharacter2(const nCharacter2& src);
    /// destructor
    virtual ~nCharacter2();
    /// get the embedded character skeleton
    nCharSkeleton& GetSkeleton();
    /// set pointer to an animation source which delivers the source data (not owned)
    void SetAnimation(nAnimation* anim);
    /// get pointer to animation source (not owned)
    const nAnimation* GetAnimation() const;
    /// set pointer to the skin animator
    void SetSkinAnimator(nSkinAnimator* animator);
    /// get pointer to the skin animator
    nSkinAnimator* GetSkinAnimator() const;
    /// set optional anim event handler (increase refcount of handler)
    void SetAnimEventHandler(nAnimEventHandler* handler);
    /// get optional anim event handler
    nAnimEventHandler* GetAnimEventHandler() const;
    /// set the currently active state
    void SetActiveState(const nAnimStateInfo& newState);
    /// get the currently active state
    const nAnimStateInfo& GetActiveState() const;
    /// evaluate the joint skeleton
    void EvaluateSkeleton(float time);
    /// emit animation events between 2 times
    void EmitAnimEvents(float startTime, float stopTime);
    /// enable/disable animation
    void SetAnimEnabled(bool b);
    /// get manual joint animation
    bool IsAnimEnabled() const;
    /// set the frame id when the character was last evaluated
    void SetLastEvaluationFrameId(uint id);
    /// get the frame id when the character was last evaluated
    uint GetLastEvaluationFrameId() const;

private:
    /// sample weighted values at a given time from nAnimation object
    bool Sample(const nAnimStateInfo& info, float time, vector4* keyArray, vector4* scratchKeyArray, int keyArraySize);
    /// emit animation events for a given time range
    void EmitAnimEvents(const nAnimStateInfo& info, float fromTime, float toTime);
    /// begin defining blended animation events
    void BeginEmitEvents();
    /// add a blended animation event
    void AddEmitEvent(const nAnimEventTrack& track, const nAnimEvent& event, float weight);
    /// finish defining blended anim events, emit the events
    void EndEmitEvents();

    enum
    {
        MaxJoints = 1024,
        MaxCurves = MaxJoints * 3,      // translate, rotate, scale per curve
    };

    nCharSkeleton charSkeleton;
    nRef<nAnimation> animation;
    nAnimEventHandler* animEventHandler;
    nSkinAnimator* skinAnimator;

    nAnimStateInfo prevStateInfo;
    nAnimStateInfo curStateInfo;

    static nArray<nAnimEventTrack> outAnimEventTracks;
    static vector4 scratchKeyArray[MaxCurves];
    static vector4 keyArray[MaxCurves];
    static vector4 transitionKeyArray[MaxCurves];

    bool animEnabled;
    uint lastEvaluationFrameId;
};

//------------------------------------------------------------------------------
/**
*/
inline
nCharSkeleton&
nCharacter2::GetSkeleton()
{
    return this->charSkeleton;
}

//------------------------------------------------------------------------------
/**
*/
inline
nSkinAnimator*
nCharacter2::GetSkinAnimator() const
{
    return this->skinAnimator;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCharacter2::SetAnimation(nAnimation* anim)
{
    n_assert(anim);
    this->animation = anim;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nAnimation*
nCharacter2::GetAnimation() const
{
    return this->animation;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCharacter2::SetAnimEnabled(bool b)
{
    this->animEnabled = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nCharacter2::IsAnimEnabled() const
{
    return this->animEnabled;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCharacter2::SetLastEvaluationFrameId(uint id)
{
    this->lastEvaluationFrameId = id;
}

//------------------------------------------------------------------------------
/**
*/
inline
uint
nCharacter2::GetLastEvaluationFrameId() const
{
    return this->lastEvaluationFrameId;
}

//------------------------------------------------------------------------------
/**
*/
inline
nAnimEventHandler*
nCharacter2::GetAnimEventHandler() const
{
    return this->animEventHandler;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nAnimStateInfo&
nCharacter2::GetActiveState() const
{
    return this->curStateInfo;
}

//------------------------------------------------------------------------------
#endif


