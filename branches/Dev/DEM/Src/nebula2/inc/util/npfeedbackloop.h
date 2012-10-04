#ifndef N_PFEEDBACKLOOP_H
#define N_PFEEDBACKLOOP_H
//------------------------------------------------------------------------------
/**
    @class nPFeedbackLoop
    @ingroup Util

    A P feedback loop (proportional feedback loop) is a simple object which
    moves a system's current state towards a goal, using the resulting error
    (difference between goal and state as feedback on the next run.

    If you need to implement motion controllers, camera controllers, etc...
    then the feedback loop is your friend.

    See Game Developer Mag issue June/July 2004.

    (C) 2004 RadonLabs GmbH
*/
#include "kernel/ntypes.h"

//------------------------------------------------------------------------------
template<class TYPE> class nPFeedbackLoop
{
public:
    /// constructor
    nPFeedbackLoop();
    /// reset the time
    void Reset(nTime time, float stepSize, float gain, const TYPE& curState);
    /// set the gain
    void SetGain(float g);
    /// get the gain
    float GetGain() const;
    /// set the goal
    void SetGoal(const TYPE& c);
    /// get the goal
    const TYPE& GetGoal() const;
    /// set the current state directly
    void SetState(const TYPE& s);
    /// get the current state the system is in
    const TYPE& GetState() const;
    /// update the object, return new state
    void Update(nTime time);

private:
    nTime time;         // the time at which the simulation is
    float stepSize;
    float gain;
    TYPE goal;
    TYPE state;
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
nPFeedbackLoop<TYPE>::nPFeedbackLoop() :
    time(0.0),
    stepSize(0.001f),
    gain(-1.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
nPFeedbackLoop<TYPE>::Reset(nTime t, float s, float g, const TYPE& curState)
{
    this->time = t;
    this->stepSize = s;
    this->gain = g;
    this->state = curState;
    this->goal = curState;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
nPFeedbackLoop<TYPE>::SetGain(float g)
{
    this->gain = g;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
float
nPFeedbackLoop<TYPE>::GetGain() const
{
    return this->gain;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
nPFeedbackLoop<TYPE>::SetGoal(const TYPE& g)
{
    this->goal = g;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
const TYPE&
nPFeedbackLoop<TYPE>::GetGoal() const
{
    return this->goal;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
nPFeedbackLoop<TYPE>::SetState(const TYPE& s)
{
    this->state = s;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
const TYPE&
nPFeedbackLoop<TYPE>::GetState() const
{
    return this->state;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
nPFeedbackLoop<TYPE>::Update(nTime curTime)
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
        this->state = this->state + (this->state - this->goal) * this->gain * this->stepSize;
        this->time += this->stepSize;
    }
}

//------------------------------------------------------------------------------
#endif
