#ifndef N_PFEEDBACKLOOP_H
#define N_PFEEDBACKLOOP_H
//------------------------------------------------------------------------------
/**
    @class nPFeedbackLoop
    @ingroup Util

    A P feedback loop (proportional feedback loop) is a simple object which
    moves a system's current State towards a Goal, using the resulting error
    (difference between Goal and State as feedback on the next run.

    If you need to implement motion controllers, camera controllers, etc...
    then the feedback loop is your friend.

    See Game Developer Mag issue June/July 2004.

    (C) 2004 RadonLabs GmbH
*/
#include "kernel/ntypes.h"

template<class T> class nPFeedbackLoop
{
private:

	nTime	Time;         // the Time at which the simulation is
	float	StepSize;

public:

	float	Gain;
	T		Goal;
	T		State;

	nPFeedbackLoop(): Time(0.0), StepSize(0.001f), Gain(-1.0f) {}

	void Reset(nTime Time, float StepSize, float Gain, const T& curState);
	void Update(nTime Time);
};

template<class T> void nPFeedbackLoop<T>::Reset(nTime t, float s, float g, const T& curState)
{
	Time = t;
	StepSize = s;
	Gain = g;
	State = curState;
	Goal = curState;
}
//---------------------------------------------------------------------

template<class T> void nPFeedbackLoop<T>::Update(nTime CurrTime)
{
	nTime dt = CurrTime - Time;

	if (dt < 0.0) Time = CurrTime;
	else if (dt > 0.5) Time = CurrTime - 0.5;

	while (Time < CurrTime)
	{
		State = State + (State - Goal) * Gain * StepSize;
		Time += StepSize;
	}
}
//---------------------------------------------------------------------

#endif
