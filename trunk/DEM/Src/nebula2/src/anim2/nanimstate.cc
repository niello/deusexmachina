//------------------------------------------------------------------------------
//  nanimstate.cc
//  (C) 2005 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "anim2/nanimstate.h"

//------------------------------------------------------------------------------
/**
    Sample the complete animation state at a given time into the
    provided float4 array. The size of the provided key array must be
    equal to the number of curves in any anim clip in the state (all clips
    must have the same number of curves).

    @param  time            the time at which to sample
    @param  animSource      the nAnimation object which provides the anim data
    @param  varContext      a variable context containing the clip weights
    @param  keyArray        pointer to a float4 array which will be filled with
                            the sampled values (one per curve)
    @param  keyArraySize    number of elements in the key array, must be identical
                            to the number of curves in any anim clip
    @return                 true, if the returned keys are valid (false if all
                            clip weights are zero)
*/
bool
nAnimState::Sample(float time,
                   nAnimation* animSource,
                   nVariableContext* varContext,
                   vector4* keyArray,
                   vector4* scratchKeyArray,
                   int keyArraySize)
{
    n_assert(animSource);
    n_assert(keyArray);
    n_assert(keyArraySize >= this->GetClipAt(0).GetNumCurves());
    n_assert(varContext);

    // first, read the clip weights from the variable context and
    // sum them up, so that we can normalize the clip weights later
    int clipIndex;
    int numClips = this->GetNumClips();
    float weightSum = 0.0f;
    for (clipIndex = 0; clipIndex < numClips; clipIndex++)
    {
        nAnimClip& clip = this->GetClipAt(clipIndex);
        float weight = 0.0f;
        nVariable* var = varContext->GetVariable(clip.GetWeightChannelHandle());
        if (var)
        {
            weight = var->GetFloat();
        }
        clip.SetWeight(weight);
        weightSum += weight;
    }
    if (weightSum > 0.0f)
    {
        // some static helper objects
        static quaternion quatCurrent;
        static quaternion quatAccum;
        static quaternion quatSlerp;

        // for each clip...
        bool firstIter = true;
        float weightAccum = 0.0f;
        for (clipIndex = 0; clipIndex < numClips; clipIndex++)
        {
            nAnimClip& clip  = this->GetClipAt(clipIndex);
            float clipWeight = clip.GetWeight() / weightSum;

            // only process clip if its weight is greater 0
            if (clipWeight > 0.0f)
            {
                // scale weightAccum so that 1 == (weightAccum + weight)
                const float scaledWeight = clipWeight / (weightAccum + clipWeight);

                // for each curve in the clip...
                int numCurves = clip.GetNumCurves();
                int firstCurveIndex = clip.GetFirstCurveIndex();

                // obtain sampled curve value for the clip's anim curve range
                animSource->SampleCurves(time, this->animGroupIndex, firstCurveIndex, numCurves, scratchKeyArray);

                int curveIndex;
                for (curveIndex = 0; curveIndex < numCurves; curveIndex++)
                {
                    int absCurveIndex = firstCurveIndex + curveIndex;
                    vector4& curArrayKey  = keyArray[curveIndex];
                    const vector4& curSampleKey = scratchKeyArray[curveIndex];

                    // perform weighted blending
                    nAnimation::Curve& animCurve = animSource->GetGroupAt(this->animGroupIndex).GetCurveAt(absCurveIndex);
                    if (animCurve.GetIpolType() == nAnimation::Curve::Quat)
                    {
                        // for quaternions, blending is a bit tricky...
                        if (firstIter)
                        {
                            // first time init of accumulators
                            curArrayKey = curSampleKey;
                        }
                        else
                        {
                            quatCurrent.set(curSampleKey.x, curSampleKey.y, curSampleKey.z, curSampleKey.w);
                            quatAccum.set(curArrayKey.x, curArrayKey.y, curArrayKey.z, curArrayKey.w);
                            quatSlerp.slerp(quatAccum, quatCurrent, scaledWeight);
                            curArrayKey.set(quatSlerp.x, quatSlerp.y, quatSlerp.z, quatSlerp.w);
                        }
                    }
                    else
                    {
                        // do normal linear blending
                        if (firstIter)
                        {
                            curArrayKey = curSampleKey * clipWeight;
                        }
                        else
                        {
                            curArrayKey += curSampleKey * clipWeight;
                        }
                    }
                }
                firstIter = false;
                weightAccum += clipWeight;
            }
        }
        return true;
    }
    else
    {
        // sum of weight is zero
        return false;
    }
}

//------------------------------------------------------------------------------
/**
    Begin blended event emission. Transformations of animation events may
    be blended just as normal animation curves. The rule is that identically
    named event tracks between several clips will be blended together if
    their events have the same timestamp.
*/
void
nAnimState::BeginEmitEvents()
{
    this->outAnimEventTracks.Clear();
}

//------------------------------------------------------------------------------
/**
    This adds a new, possibly blended animation event.
*/
void
nAnimState::AddEmitEvent(const nAnimEventTrack& track, const nAnimEvent& event, float weight)
{
    // check if track already exists
    int trackIndex;
    int numTracks = this->outAnimEventTracks.Size();
    for (trackIndex = 0; trackIndex < numTracks; trackIndex++)
    {
        if (this->outAnimEventTracks[trackIndex].GetName() == track.GetName())
        {
            break;
        }
    }
    if (trackIndex == numTracks)
    {
        // track didn't exist
        nAnimEventTrack newTrack;
        newTrack.SetName(track.GetName());
        this->outAnimEventTracks.Append(newTrack);
    }

    // check if event must be blended, this is the case if an event with an
    // identical time stamp already exists in this track
    int eventIndex;
    int numEvents = this->outAnimEventTracks[trackIndex].GetNumEvents();
    for (eventIndex = 0; eventIndex < numEvents; eventIndex++)
    {
        float time0 = this->outAnimEventTracks[trackIndex].GetEvent(eventIndex).GetTime();
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

        this->outAnimEventTracks[trackIndex].AddEvent(newEvent);
    }
    else
    {
        // perform transformation blending
        nAnimEvent& blendEvent = this->outAnimEventTracks[trackIndex].EventArray()[eventIndex];
        blendEvent.SetTranslation(blendEvent.GetTranslation() + event.GetTranslation() * weight);
        blendEvent.SetScale(blendEvent.GetScale() + event.GetScale() * weight);

        // proper quaternion blending is a bit tricky
/*
        const float scaledWeight = weight / (blendEvent.GetWeightAccum() + weight);
        quaternion q;
        q.slerp(blendEvent.GetQuaternion(), event.GetQuaternion(), scaledWeight);
        blendEvent.SetQuaternion(q);
        blendEvent.SetWeightAccum(blendEvent.GetWeightAccum() + weight);
*/
    }
}

//------------------------------------------------------------------------------
/**
    Finish defining anim event emission. This is where the events
    will actually be transferred to the anim event handler.
*/
void
nAnimState::EndEmitEvents(nAnimEventHandler* handler)
{
    n_assert(handler);
    int numTracks = this->outAnimEventTracks.Size();
    int trackIndex;
    for (trackIndex = 0; trackIndex < numTracks; trackIndex++)
    {
        int numEvents = this->outAnimEventTracks[trackIndex].GetNumEvents();
        int eventIndex;
        for (eventIndex = 0; eventIndex < numEvents; eventIndex++)
        {
            handler->HandleEvent(this->outAnimEventTracks[trackIndex], eventIndex);
        }
    }
}

//------------------------------------------------------------------------------
/**
    This emits animation events through the given animation event handler
    from the provided start time to the provided end time (should be
    the last time that the animation has been sampled, and the current time.

    Anim event transformations will be weight-mixed.
*/
void
nAnimState::EmitAnimEvents(float fromTime, float toTime, nAnimation* animSource, nAnimEventHandler* handler)
{
    n_assert(handler);
    n_assert(animSource);

    float timeDiff = toTime - fromTime;
    if ((timeDiff <= 0.0f) || (timeDiff > 0.25f))
    {
        // a time exception, just do nothing
        return;
    }

    // for each clip with a weight > 0.0...
    this->BeginEmitEvents();
    const nAnimation::Group& animGroup = animSource->GetGroupAt(this->animGroupIndex);
    int clipIndex;
    int numClips = this->GetNumClips();
    for (clipIndex = 0; clipIndex < numClips; clipIndex++)
    {
        nAnimClip& clip = this->GetClipAt(clipIndex);
        if (clip.GetWeight() > 0.0f)
        {
            int numTracks = clip.GetNumAnimEventTracks();
            int trackIndex;
            for (trackIndex = 0; trackIndex < numTracks; trackIndex++)
            {
                nAnimEventTrack& track = clip.GetAnimEventTrackAt(trackIndex);
                int numEvents = track.GetNumEvents();
                int eventIndex;
                for (eventIndex = 0; eventIndex < numEvents; eventIndex++)
                {
                    const nAnimEvent& event = track.GetEvent(eventIndex);
                    if (animGroup.IsInbetween(event.GetTime(), fromTime, toTime))
                    {
                        this->AddEmitEvent(track, track.GetEvent(eventIndex), clip.GetWeight());
                    }
                }
            }
        }
    }
    this->EndEmitEvents(handler);
}

