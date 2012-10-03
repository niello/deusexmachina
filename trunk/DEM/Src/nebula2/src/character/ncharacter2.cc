//------------------------------------------------------------------------------
//  ncharacter2.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "character/ncharacter2.h"
#include "anim2/nanimeventhandler.h"
#include "scene/nskinanimator.h"
#include "variable/nvariablecontext.h"

nArray<nAnimEventTrack> nCharacter2::outAnimEventTracks;
vector4 nCharacter2::scratchKeyArray[MaxCurves];
vector4 nCharacter2::keyArray[MaxCurves];
vector4 nCharacter2::transitionKeyArray[MaxCurves];

//------------------------------------------------------------------------------
/**
*/
nCharacter2::nCharacter2() :
    animEnabled(true),
    lastEvaluationFrameId(0),
    skinAnimator(0),
    animEventHandler(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nCharacter2::nCharacter2(const nCharacter2& src) :
    animEnabled(true)
{
    *this = src;
}

//------------------------------------------------------------------------------
/**
*/
nCharacter2::~nCharacter2()
{
    this->SetSkinAnimator(0);
    this->SetAnimEventHandler(0);
}


//------------------------------------------------------------------------------
/**
*/
void
nCharacter2::SetSkinAnimator(nSkinAnimator* animator)
{
    if (this->skinAnimator)
    {
        this->skinAnimator->Release();
        this->skinAnimator = 0;
    }
    if (animator)
    {
        this->skinAnimator = animator;
        this->skinAnimator->AddRef();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
nCharacter2::SetAnimEventHandler(nAnimEventHandler* handler)
{
    if (this->animEventHandler)
    {
        this->animEventHandler->Release();
        this->animEventHandler = 0;
    }
    if (handler)
    {
        this->animEventHandler = handler;
        this->animEventHandler->AddRef();
    }
}

//------------------------------------------------------------------------------
/**
    Set a new animation state, and handle stuff necessary for
    blending between previous and current state.
*/
void
nCharacter2::SetActiveState(const nAnimStateInfo& newState)
{
    this->prevStateInfo = this->curStateInfo;
    this->curStateInfo = newState;
}

//------------------------------------------------------------------------------
/**
*/
void
nCharacter2::EvaluateSkeleton(float time)
{
    if (this->IsAnimEnabled() && this->curStateInfo.IsValid())
    {
        n_assert(this->animation);

        // check if a state transition is necessary
        float curRelTime = time - this->curStateInfo.GetStateStarted();

        // handle time exception (this happens when time is reset to a smaller value
        // since the last animation state switch)
        if (curRelTime < 0.0f)
        {
            curRelTime = 0.0f;
            this->curStateInfo.SetStateStarted(time);
        }

        int numClips = this->curStateInfo.GetNumClips();
        int clipIndex;
        for (clipIndex  = 0; clipIndex < numClips; clipIndex++)
        {
            if (this->curStateInfo.GetClipAt(clipIndex).GetClipName() != "baseClip")
            {
                int index = this->curStateInfo.GetClipAt(clipIndex).GetAnimGroupIndex();
                const nAnimation::Group& group = this->animation->GetGroupAt(0);

                float fadeInTime = this->curStateInfo.GetFadeInTime();
                float lerp = 1.0f;
                bool transition = false;
                if ((fadeInTime > 0.0f) && (curRelTime < fadeInTime) && this->prevStateInfo.IsValid())
                {
                    // state transition is necessary, compute a lerp value
                    // and sample the previous animation state
                    float prevRelTime = time - this->prevStateInfo.GetStateStarted();
                    float sampleTime = prevRelTime + this->prevStateInfo.GetStateOffset();
                    if (this->Sample(this->prevStateInfo, sampleTime, nCharacter2::transitionKeyArray, nCharacter2::scratchKeyArray, nCharacter2::MaxCurves))
                    {
                         transition = true;
                         lerp = curRelTime / fadeInTime;
                    }
                }

               // get samples from current animation state
               float sampleTime = curRelTime + this->curStateInfo.GetStateOffset();
               if (this->Sample(this->curStateInfo, sampleTime, nCharacter2::keyArray, nCharacter2::scratchKeyArray, nCharacter2::MaxCurves))
               {
                     // transfer the sampled animation values into the character skeleton
                     int numJoints = this->charSkeleton.GetNumJoints();
                     int jointIndex;
                     const vector4* keyPtr = keyArray;
                     const vector4* prevKeyPtr = transitionKeyArray;

                     vector3 translate, prevTranslate;
                     quaternion rotate, prevRotate;
                     vector3 scale, prevScale;
                     for (jointIndex = 0; jointIndex < numJoints; jointIndex++)
                     {
                          // read sampled translation, rotation and scale
                          translate.set(keyPtr->x, keyPtr->y, keyPtr->z);          keyPtr++;
                          rotate.set(keyPtr->x, keyPtr->y, keyPtr->z, keyPtr->w);  keyPtr++;
                          scale.set(keyPtr->x, keyPtr->y, keyPtr->z);              keyPtr++;

                          if (transition)
                          {
                               prevTranslate.set(prevKeyPtr->x, prevKeyPtr->y, prevKeyPtr->z);              prevKeyPtr++;
                               prevRotate.set(prevKeyPtr->x, prevKeyPtr->y, prevKeyPtr->z, prevKeyPtr->w);  prevKeyPtr++;
                               prevScale.set(prevKeyPtr->x, prevKeyPtr->y, prevKeyPtr->z);                  prevKeyPtr++;
                               translate.lerp(prevTranslate, lerp);
                               rotate.slerp(prevRotate, rotate, lerp);
                               scale.lerp(prevScale, lerp);
                           }

                           nCharJoint& joint = this->charSkeleton.GetJointAt(jointIndex);
                           joint.SetTranslate(translate);
                           joint.SetRotate(rotate);
                           joint.SetScale(scale);
                      }
                 }
            }
        }
    }
    this->charSkeleton.Evaluate();
}

//------------------------------------------------------------------------------
/**
    Emit animation event for the current animation states.
*/
void
nCharacter2::EmitAnimEvents(float fromTime, float toTime)
{
    if (this->animEventHandler && this->curStateInfo.IsValid())
    {
        n_assert(this->animation);
        float relFromTime = (fromTime - this->curStateInfo.GetStateStarted()) + this->curStateInfo.GetStateOffset();
        float relToTime   = (toTime - this->curStateInfo.GetStateStarted()) + this->curStateInfo.GetStateOffset();
        this->EmitAnimEvents(this->curStateInfo, relFromTime, relToTime);
    }
}

//------------------------------------------------------------------------------
/**
    Sample the complete animation state at a given time into the
    provided float4 array. The size of the provided key array must be
    equal to the number of curves in any animation clip in the state (all clips
    must have the same number of curves).

    @param  time            the time at which to sample
    @param  keyArray        pointer to a float4 array which will be filled with
                            the sampled values (one per curve)
    @param  keyArraySize    number of elements in the key array, must be identical
                            to the number of curves in any animation clip
    @return                 true, if the returned keys are valid (false if all
                            clip weights are zero)
*/
bool
nCharacter2::Sample(const nAnimStateInfo& stateInfo, float time, vector4* keyArray, vector4* scratchKeyArray, int keyArraySize)
{
    n_assert(keyArray);
    n_assert(keyArraySize >= stateInfo.GetClipAt(0).GetNumCurves());
    n_assert(this->animation.isvalid());

    // some static helper objects
    static quaternion quatCurrent;
    static quaternion quatAccum;
    static quaternion quatSlerp;

    // for each clip...
    float weightAccum = 0.0f;
    int clipIndex;
    const int numClips = stateInfo.GetNumClips();
    for (clipIndex = 0; clipIndex < numClips; clipIndex++)
    {
        const nAnimClip& clip  = stateInfo.GetClipAt(clipIndex);
        const float clipWeight = stateInfo.GetClipWeightAt(clipIndex);
        const float scaledWeight = clipWeight / (weightAccum + clipWeight); // scale weightAccum so that 1 == (weightAccum + weight)
        const int animGroupIndex = clip.GetAnimGroupIndex();
        const nAnimation::Group& group = this->animation->GetGroupAt(animGroupIndex);
        const int numCurves = group.GetNumCurves();

        // obtain sampled curve value for the clip's animation curve range
        this->animation->SampleCurves(time, animGroupIndex, 0, numCurves, scratchKeyArray);

        int curveIndex;
        for (curveIndex = 0; curveIndex < numCurves; curveIndex++)
        {
            vector4& curMixedKey = keyArray[curveIndex];
            const vector4& curSampleKey = scratchKeyArray[curveIndex];

            // perform weighted blending
            nAnimation::Curve& animCurve = this->animation->GetGroupAt(animGroupIndex).GetCurveAt(curveIndex);
            if (animCurve.IsAnimated() && clipWeight > 0.0f)
            {
                // FIXME: (for cases with more than two clips) maybe all weights of animated curves
                // have to be summed up (for every frame)

                // check all curves from previous clips at current curve index if they are animated
                bool animFlag = false;
                int i;
                for (i = 0; i < clipIndex; i++)
                {
                    nAnimClip& clip = stateInfo.GetClipAt(i);
                    int animGroupIndex = clip.GetAnimGroupIndex();
                    nAnimation::Curve& prevClipCurve = this->animation->GetGroupAt(animGroupIndex).GetCurveAt(curveIndex);
                    if (prevClipCurve.IsAnimated())
                    {
                        animFlag = true;
                        break;
                    }
                }

                if (!animFlag)
                {
                    // no previous curves are animated; take my current sample key
                    if (animCurve.GetIpolType() == nAnimation::Curve::Quat)
                    {
                        curMixedKey = curSampleKey;
                    }
                    else
                    {
                        curMixedKey = curSampleKey * clipWeight;
                    }
                }
                else
                {
                    // lerp my current sample key with previous curves
                   if (animCurve.GetIpolType() == nAnimation::Curve::Quat)
                   {
                        quatCurrent.set(curSampleKey.x, curSampleKey.y, curSampleKey.z, curSampleKey.w);
                        quatAccum.set(curMixedKey.x, curMixedKey.y, curMixedKey.z, curMixedKey.w);
                        quatSlerp.slerp(quatAccum, quatCurrent, scaledWeight);
                        curMixedKey.set(quatSlerp.x, quatSlerp.y, quatSlerp.z, quatSlerp.w);
                   }
                   else
                   {
                        curMixedKey += curSampleKey * clipWeight;
                   }
                }

                animCurve.SetCurAnimClipValue(curSampleKey);
             }
             else // curve is not animated
             {
                 // FIXME: (for cases with more than two clips) maybe all weights of animated curves
                 // have to be summed up (for every frame)

                 // check all curves from previous clips at current curve index if they are animated
                 vector4 startVal;
                 bool animFlag = false;
                 int i;
                 for (i = 0; i < clipIndex; i++)
                 {
                     nAnimClip& clip = stateInfo.GetClipAt(i);
                     int animGroupIndex = clip.GetAnimGroupIndex();
                     nAnimation::Curve& prevClipCurve = this->animation->GetGroupAt(animGroupIndex).GetCurveAt(curveIndex);
                     if (prevClipCurve.IsAnimated())
                     {
                         // start value from prev animated curve taken
                         animFlag = true;
                         startVal = prevClipCurve.GetCurAnimClipValue();
                     }
                 }

                 if (!animFlag)
                 {
                     // no other curves are animated, take the start value of the current curve
                     if (animCurve.GetIpolType() == nAnimation::Curve::Quat)
                     {
                        curMixedKey = animCurve.GetStartValue();
                     }
                     else
                     {
                        curMixedKey = animCurve.GetStartValue() * clipWeight;
                     }
                 }
                 else
                 {
                     // lerp my start value key with previous curves
                     if (animCurve.GetIpolType() == nAnimation::Curve::Quat)
                     {
                        quatCurrent.set(startVal.x, startVal.y, startVal.z, startVal.w);
                        quatAccum.set(curMixedKey.x, curMixedKey.y, curMixedKey.z, curMixedKey.w);
                        quatSlerp.slerp(quatAccum, quatCurrent, scaledWeight);
                        curMixedKey.set(quatSlerp.x, quatSlerp.y, quatSlerp.z, quatSlerp.w);
                     }
                     else
                     {
                        curMixedKey += startVal * clipWeight;
                     }
                 }
             }
        }
        weightAccum += clipWeight;
    }

    return true;
}

//------------------------------------------------------------------------------
/**
    Begin blended event emission. Transformations of animation events may
    be blended just as normal animation curves. The rule is that identically
    named event tracks between several clips will be blended together if
    their events have the same timestamp.
*/
void
nCharacter2::BeginEmitEvents()
{
    nCharacter2::outAnimEventTracks.Clear();
}

//------------------------------------------------------------------------------
/**
    This adds a new, possibly blended animation event.
*/
void
nCharacter2::AddEmitEvent(const nAnimEventTrack& track, const nAnimEvent& event, float weight)
{
    // check if track already exists
    int trackIndex;
    int numTracks = nCharacter2::outAnimEventTracks.Size();
    for (trackIndex = 0; trackIndex < numTracks; trackIndex++)
    {
        if (nCharacter2::outAnimEventTracks[trackIndex].GetName() == track.GetName())
        {
            break;
        }
    }
    if (trackIndex == numTracks)
    {
        // track didn't exist
        nAnimEventTrack newTrack;
        newTrack.SetName(track.GetName());
        nCharacter2::outAnimEventTracks.Append(newTrack);
    }

    // check if event must be blended, this is the case if an event with an
    // identical time stamp already exists in this track
    int eventIndex;
    int numEvents = nCharacter2::outAnimEventTracks[trackIndex].GetNumEvents();
    for (eventIndex = 0; eventIndex < numEvents; eventIndex++)
    {
        float time0 = nCharacter2::outAnimEventTracks[trackIndex].GetEvent(eventIndex).GetTime();
        float time1 = event.GetTime();
        if (n_fequal(time0, time1, 0.01f))
        {
            break;
        }
    }
    if (eventIndex == numEvents)
    {
        // add a new event...
        nAnimEvent newEvent;
        newEvent.SetTime(event.GetTime());
        newEvent.SetTranslation(event.GetTranslation() * weight);
        newEvent.SetScale(event.GetScale() * weight);
        newEvent.SetQuaternion(event.GetQuaternion());

        nCharacter2::outAnimEventTracks[trackIndex].AddEvent(newEvent);
    }
    else
    {
        // perform transformation blending
        nAnimEvent& blendEvent = nCharacter2::outAnimEventTracks[trackIndex].EventArray()[eventIndex];
        blendEvent.SetTranslation(blendEvent.GetTranslation() + event.GetTranslation() * weight);
        blendEvent.SetScale(blendEvent.GetScale() + event.GetScale() * weight);

        // proper quaternion blending is a bit tricky
        //const float scaledWeight = weight / (blendEvent.GetWeightAccum() + weight);
        //quaternion q;
        //q.slerp(blendEvent.GetQuaternion(), event.GetQuaternion(), scaledWeight);
        //blendEvent.SetQuaternion(q);
        //blendEvent.SetWeightAccum(blendEvent.GetWeightAccum() + weight);
    }
}

//------------------------------------------------------------------------------
/**
    Finish defining animation event emission. This is where the events
    will actually be transferred to the animation event handler.
*/
void
nCharacter2::EndEmitEvents()
{
    n_assert(0 != this->animEventHandler);

    int numTracks = nCharacter2::outAnimEventTracks.Size();
    int trackIndex;
    for (trackIndex = 0; trackIndex < numTracks; trackIndex++)
    {
        int numEvents = nCharacter2::outAnimEventTracks[trackIndex].GetNumEvents();
        int eventIndex;
        for (eventIndex = 0; eventIndex < numEvents; eventIndex++)
        {
            this->animEventHandler->HandleEvent(nCharacter2::outAnimEventTracks[trackIndex], eventIndex);
        }
    }
}

//------------------------------------------------------------------------------
/**
    This emits animation events through the given animation event handler
    from the provided start time to the provided end time (should be
    the last time that the animation has been sampled, and the current time.

    Animation event transformations will be weight-mixed.
*/
void
nCharacter2::EmitAnimEvents(const nAnimStateInfo& stateInfo, float fromTime, float toTime)
{
    n_assert(this->animation.isvalid());
    n_assert(0 != this->animEventHandler);

    float timeDiff = toTime - fromTime;
    if ((timeDiff <= 0.0f) || (timeDiff > 0.25f))
    {
        // a time exception, just do nothing
        return;
    }

    // for each clip with a weight > 0.0...
    this->BeginEmitEvents();

    int clipIndex;
    int numClips = stateInfo.GetNumClips();
    for (clipIndex = 0; clipIndex < numClips; clipIndex++)
    {
        const nAnimClip& clip = stateInfo.GetClipAt(clipIndex);
        const float clipWeight = stateInfo.GetClipWeightAt(clipIndex);
        const nAnimation::Group& animGroup = this->animation->GetGroupAt(stateInfo.GetClipAt(clipIndex).GetAnimGroupIndex());
        if (clipWeight > 0.0f)
        {
            int numTracks = clip.GetNumAnimEventTracks();
            int trackIndex;
            for (trackIndex = 0; trackIndex < numTracks; trackIndex++)
            {
                const nAnimEventTrack& track = clip.GetAnimEventTrackAt(trackIndex);
                int numEvents = track.GetNumEvents();
                int eventIndex;
                for (eventIndex = 0; eventIndex < numEvents; eventIndex++)
                {
                    const nAnimEvent& event = track.GetEvent(eventIndex);
                    if (animGroup.IsInbetween(event.GetTime(), fromTime, toTime))
                    {
                        this->AddEmitEvent(track, track.GetEvent(eventIndex), clipWeight);
                    }
                }
            }
        }
    }

    // handle all events
    this->EndEmitEvents();
}

