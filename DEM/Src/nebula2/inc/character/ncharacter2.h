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

class nSkinAnimator;
class nAnimEventHandler;

class nCharacter2: public nRefCounted
{
public:

	bool animEnabled;
    uint LastEvalFrame;

	nCharacter2(): animEnabled(true), LastEvalFrame(0), pSkinAnimator(NULL), pEvtHandler(NULL) {}
    /// copy constructor
    nCharacter2(const nCharacter2& src): animEnabled(true) { *this = src; }
    /// destructor
	virtual ~nCharacter2() { SetSkinAnimator(NULL); SetAnimEventHandler(NULL); }
    /// get the embedded character skeleton
	nCharSkeleton& GetSkeleton() { return charSkeleton; }
    /// set pointer to an animation source which delivers the source data (not owned)
	void SetAnimation(nAnimation* anim) { n_assert(anim); animation = anim; }
    /// get pointer to animation source (not owned)
	const nAnimation* GetAnimation() const { return animation; }
    /// set pointer to the skin animator
    void SetSkinAnimator(nSkinAnimator* animator);
    /// get pointer to the skin animator
	nSkinAnimator* GetSkinAnimator() const { return pSkinAnimator; }
    /// set optional anim event handler (increase refcount of handler)
    void SetAnimEventHandler(nAnimEventHandler* handler);
    /// get optional anim event handler
	nAnimEventHandler* GetAnimEventHandler() const { return pEvtHandler; }
    /// set the currently active state
    void SetActiveState(const nAnimStateInfo& newState);
    /// get the currently active state
	const nAnimStateInfo& GetActiveState() const { return curStateInfo; }
    /// evaluate the joint skeleton
    void EvaluateSkeleton(float time);
    /// emit animation events between 2 times
    void EmitAnimEvents(float startTime, float stopTime);

private:
    /// sample weighted values at a given time from nAnimation object
    bool Sample(const nAnimStateInfo& info, float time, vector4* keyArray, vector4* scratchKeyArray, int keyArraySize);
    /// emit animation events for a given time range
    void EmitAnimEvents(const nAnimStateInfo& info, float fromTime, float toTime);
    /// add a blended animation event
    void AddEmitEvent(const nAnimEventTrack& track, const nAnimEvent& event, float weight);

    enum
    {
        MaxJoints = 256,
        MaxCurves = MaxJoints * 3,      // translate, rotate, scale per curve
    };

    nCharSkeleton charSkeleton;
    nRef<nAnimation> animation;
    nAnimEventHandler* pEvtHandler;
    nSkinAnimator* pSkinAnimator;

    nAnimStateInfo prevStateInfo;
    nAnimStateInfo curStateInfo;

    static nArray<nAnimEventTrack> outAnimEventTracks;
    static vector4 scratchKeyArray[MaxCurves];
    static vector4 keyArray[MaxCurves];
    static vector4 transitionKeyArray[MaxCurves];
};

#endif


