#ifndef N_PQUATFEEDBACKLOOP_H
#define N_PQUATFEEDBACKLOOP_H
//------------------------------------------------------------------------------
/**
    @class nPQuatFeedbackLoop
    @ingroup Util

    A specialized proportional feedback loop for rotations, using a
    quaternion representation.

    (C) 2004 RadonLabs GmbH
*/
#include "kernel/ntypes.h"
#include "mathlib/quaternion.h"

//------------------------------------------------------------------------------
class nPQuatFeedbackLoop
{
public:
    /// constructor
    nPQuatFeedbackLoop();
    /// reset the time
    void Reset(nTime time, float stepSize, float gain, const quaternion& curState);
    /// set the gain
    void SetGain(float g);
    /// get the gain
    float GetGain() const;
    /// set the goal
    void SetGoal(const quaternion& c);
    /// get the goal
    const quaternion& GetGoal() const;
    /// set the current state directly
    void SetState(const quaternion& s);
    /// get the current state the system is in
    const quaternion& GetState() const;
    /// update the object, return new state
    void Update(nTime time);

private:
    nTime time;         // the time at which the simulation is
    float stepSize;
    float gain;
    quaternion goal;
    quaternion state;
};

//------------------------------------------------------------------------------
/**
*/
inline
nPQuatFeedbackLoop::nPQuatFeedbackLoop() :
    time(0.0),
    stepSize(0.001f),
    gain(-1.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nPQuatFeedbackLoop::Reset(nTime t, float s, float g, const quaternion& curState)
{
    this->time = t;
    this->stepSize = s;
    this->gain = g;
    this->state = curState;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nPQuatFeedbackLoop::SetGain(float g)
{
    n_assert(g > 0.0f);
    this->gain = g;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nPQuatFeedbackLoop::GetGain() const
{
    return this->gain;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nPQuatFeedbackLoop::SetGoal(const quaternion& g)
{
    this->goal = g;
}

//------------------------------------------------------------------------------
/**
*/
inline
const quaternion&
nPQuatFeedbackLoop::GetGoal() const
{
    return this->goal;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nPQuatFeedbackLoop::SetState(const quaternion& s)
{
    this->state = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
const quaternion&
nPQuatFeedbackLoop::GetState() const
{
    return this->state;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nPQuatFeedbackLoop::Update(nTime curTime)
{
    nTime dt = curTime - this->time;

    // catch time exceptions
    if (dt < 0.0)
    {
        this->time = curTime;
    }
    else if (dt > 0.5)
    {
        this->time = curTime;
    }

    while (this->time < curTime)
    {
        // compute angular error between state and goal
        vector3 stateVec = this->state.rotate(vector3(0.0f, 0.0f, 1.0f));
        vector3 goalVec  = this->goal.rotate(vector3(0.0f, 0.0f, 1.0f));
        float dot = stateVec % goalVec;

        // dot is now between -1 (opposite vectors) and 1 (identical vectors),
        // generate a lerp value between 0 (represents goal) and 1 (represents state)
        float error = (-dot + 1.0f) * 0.5f;

        // error now 0 for "no error" and 1 for "max error"
        float lerp = error * this->gain * this->stepSize;
        quaternion res;
        res.slerp(this->state, this->goal, lerp);
        this->state = res;

        this->time += this->stepSize;
    }
}

//------------------------------------------------------------------------------
#endif