#ifndef N_ANGULARPFEEDBACKLOOP_H
#define N_ANGULARPFEEDBACKLOOP_H
//------------------------------------------------------------------------------
/**
    @class nAngularPFeedbackLoop
    @ingroup Util

    A proportional feedback loop with correct angular interpolation.

    (C) 2004 RadonLabs GmbH
*/
#include "kernel/ntypes.h"

//------------------------------------------------------------------------------
class nAngularPFeedbackLoop
{
public:
    /// constructor
    nAngularPFeedbackLoop();
    /// reset the time
    void Reset(nTime time, float stepSize, float gain, float curState);
    /// set the gain
    void SetGain(float g);
    /// get the gain
    float GetGain() const;
    /// set the goal
    void SetGoal(float c);
    /// get the goal
    float GetGoal() const;
    /// set the current state directly
    void SetState(float s);
    /// get the current state the system is in
    float GetState() const;
    /// update the object, return new state
    void Update(nTime time);

private:
    /// compute shortest angular distance between 2 angles
    float AngularDistance(float from, float to) const;

    nTime time;         // the time at which the simulation is
    float stepSize;
    float gain;
    float goal;
    float state;
};

//------------------------------------------------------------------------------
/**
*/
inline
nAngularPFeedbackLoop::nAngularPFeedbackLoop() :
    time(0.0),
    stepSize(0.001f),
    gain(-1.0f),
    goal(0.0f),
    state(0.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAngularPFeedbackLoop::Reset(nTime t, float s, float g, float curState)
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
nAngularPFeedbackLoop::SetGain(float g)
{
    this->gain = g;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nAngularPFeedbackLoop::GetGain() const
{
    return this->gain;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAngularPFeedbackLoop::SetGoal(float g)
{
    this->goal = g;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nAngularPFeedbackLoop::GetGoal() const
{
    return this->goal;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAngularPFeedbackLoop::SetState(float s)
{
    this->state = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nAngularPFeedbackLoop::GetState() const
{
    return this->state;
}

//------------------------------------------------------------------------------
/**
    Compute the shortest angular distance between 2 angles. The angular distance
    will be between rad(-180) and rad(180). Positive distance are in
    counter-clockwise order, negative distances in clockwise order.
*/
inline
float
nAngularPFeedbackLoop::AngularDistance(float from, float to) const
{
    float nFrom = n_normangle(from);
    float nTo   = n_normangle(to);
    float dist = nTo - nFrom;
    if (dist < n_deg2rad(-180.0f))
    {
        dist += n_deg2rad(360.0f);
    }
    else if (dist >= n_deg2rad(180.0f))
    {
        dist -= n_deg2rad(360.0f);
    }
    return dist;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAngularPFeedbackLoop::Update(nTime curTime)
{
    nTime dt = curTime - this->time;

    // catch time exceptions
    if (dt < 0.0)
    {
        this->time = curTime;
    }
    else if (dt > 0.5)
    {
        this->time = curTime - 0.5;
    }

    while (this->time < curTime)
    {
        // get angular distance error
        float error = n_angulardistance(this->state, this->goal);
        if (n_abs(error) > N_TINY)
        {
            this->state = n_normangle(this->state - (error * this->gain * this->stepSize));
            this->time += this->stepSize;
        }
        else
        {
            this->state = this->goal;
            this->time = curTime;
            break;
        }
    }
}

//------------------------------------------------------------------------------
#endif
