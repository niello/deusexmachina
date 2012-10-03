#ifndef N_PIDFEEDBACKLOOP_H
#define N_PIDFEEDBACKLOOP_H
//------------------------------------------------------------------------------
/**
    @class nPIDFeedbackLoop
    @ingroup Util

    A PID feedback loop (proportional integral derivative feedback loop)

    (C) 2006 RadonLabs GmbH
*/
#include "kernel/ntypes.h"

class nPIDFeedbackLoop
{
public:
    /// constructor
    nPIDFeedbackLoop();
    /// set value of loop
    void SetValue(double value);
    /// set the goal
    void SetGoal(double wantedValue);
    /// set the propotional, integral and derivative constants, and maximum acceleration (how fast the value kann change, will be disabled if set to 0.0 (default))
    void SetConstants(double pConst, double iConst, double dConst, double acceleration = 0.0);
    /// get current value
    const double& GetValue() const;
    /// get the goal
    const double& GetGoal() const;
    /// get last computed error
    double GetLastError() const;
    /// last delta of error
    double GetLastDelta() const;
    /// update current value
    void Update(nTime time);
    /// reset running error
    void ResetError();

private:
    double value;               // current value of the controller
    double goal;            // the value the controller is trying to achieve

    double pConst;              // proportional constant (Kp)
    double iConst;              // integral constant (Ki)
    double dConst;              // derivative constant (Kd)
    double maxAcceleration;     // limits how fast the control can accelerate the value

    double lastError;           // previous error
    double lastDelta;           // amount of change during last adjustment
    double runningError;        // summed errors (using as the integral value)
    bool validError;            // prevents numerical problems on the first adjustment

    double lastDeltaTime;

    double maxAllowableDeltaTime;   // if more time (in seconds) than this has passed, no PID adjustments will be made
};

//------------------------------------------------------------------------------
/**
*/
inline
nPIDFeedbackLoop::nPIDFeedbackLoop() :
    value(0.0),
    goal(0.0),
    pConst(1.0),
    iConst(0.0),
    dConst(0.0),
    runningError(0.0),
    lastDelta(0.0),
    lastError(0.0),
    validError(false),
    maxAcceleration(0.0),
    lastDeltaTime(0.0),
    maxAllowableDeltaTime(0.03)
{
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nPIDFeedbackLoop::SetValue(double value)
{
    this->value = value;
    this->lastError = 0.0;
    this->lastDelta = 0.0;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nPIDFeedbackLoop::SetGoal(double wantedValue)
{
    this->goal = wantedValue;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nPIDFeedbackLoop::SetConstants(double pConst, double iConst, double dConst, double acceleration)
{
    this->pConst = pConst;
    this->iConst = iConst;
    this->dConst = dConst;
    maxAcceleration = acceleration;
}

//------------------------------------------------------------------------------
/**
*/
inline
const double&
nPIDFeedbackLoop::GetValue() const
{
    return this->value;
}


//------------------------------------------------------------------------------
/**
*/
inline
double
nPIDFeedbackLoop::GetLastError() const
{
    return this->lastError;
}

//------------------------------------------------------------------------------
/**
*/
inline
const double&
nPIDFeedbackLoop::GetGoal() const
{
    return this->goal;
}

//------------------------------------------------------------------------------
/**
*/
inline
double
nPIDFeedbackLoop::GetLastDelta() const
{
    return this->lastDelta;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nPIDFeedbackLoop::Update(nTime time)
{
    // if too much time has passed, do nothing
    if (time != 0.0f)
    {
        if (time > maxAllowableDeltaTime)
            time = maxAllowableDeltaTime;

        // compute the error and sum of the errors for the integral
        double error = (goal - value) * time;
        runningError += error;

        // proportional
        double dP = pConst * error;

        // integral
        double dI = iConst * runningError * time;

        // derivative
        double dD(0.0f);
        if (validError)
            dD = dConst * (lastError - error) * time;
        else
            validError = true;

        // remember the error for derivative
        lastError = error;

        // compute the adjustment
        double thisDelta = dP + dI + dD;

        // clamp the acceleration
        if (maxAcceleration != 0.0f || false)
        {
            double timeRatio(1.0);
            if (lastDeltaTime != 0.0)
                timeRatio = time / lastDeltaTime;
            lastDeltaTime = time;

            lastDelta *= timeRatio;
            double difference = (thisDelta - lastDelta);
            double accl = maxAcceleration * time * time;

            if (difference < -accl)
                thisDelta = lastDelta - accl;
            else if (difference > accl)
                thisDelta = lastDelta + accl;
        }

        // modify the value
        value += thisDelta;
        lastDelta = thisDelta;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nPIDFeedbackLoop::ResetError()
{
    runningError = 0.0f;
    validError = false;
}

//------------------------------------------------------------------------------
#endif