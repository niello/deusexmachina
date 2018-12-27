#ifndef N_PFEEDBACKLOOP_H
#define N_PFEEDBACKLOOP_H

#include <StdDEM.h>

// A PFLoop (proportional feedback loop) is a simple object which moves a system's
// current State towards a Goal, using the resulting error (difference between Goal
// and State) as feedback on the next run.
// If you need to implement motion controllers, camera controllers, etc then the
// feedback loop is your friend. See Game Developer Mag issue June/July 2004.
// (C) 2004 RadonLabs GmbH

template<class T>
class CPFLoop
{
private:

	float	CurrTime;
	float	Step;

public:

	float	Gain;
	T		Goal;
	T		State;

	CPFLoop(): CurrTime(0.f), Step(0.001f), Gain(-1.f), Goal(0.f), State(0.f) {}

	void Reset(float Time, float StepSize) { CurrTime = Time; Step = StepSize; }
	void Update(float Time);
};

template<class T> void CPFLoop<T>::Update(float Time)
{
	float dt = Time - CurrTime;
	if (dt < 0.f) CurrTime = Time;
	else if (dt > 0.5f) CurrTime = Time - 0.5f;

	while (CurrTime < Time)
	{
		State = State + (State - Goal) * Gain * Step;
		CurrTime += Step;
	}
}
//---------------------------------------------------------------------

#endif
