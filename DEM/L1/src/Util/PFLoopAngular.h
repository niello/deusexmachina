#ifndef N_ANGULARPFEEDBACKLOOP_H
#define N_ANGULARPFEEDBACKLOOP_H

#include <StdDEM.h>

// A proportional feedback loop with correct angular interpolation
// (C) 2004 RadonLabs GmbH

class CPFLoopAngular
{
private:

	float CurrTime;
	float Step;

public:

	float Gain;
	float Goal;
	float State;

	CPFLoopAngular(): CurrTime(0.f), Step(0.001f), Gain(-1.f), Goal(0.f), State(0.f) {}

	void Reset(float Time, float StepSize) { CurrTime = Time; Step = StepSize; }
    void Update(float Time);
};

inline void CPFLoopAngular::Update(float Time)
{
	float dt = Time - CurrTime;
	if (dt < 0.f) CurrTime = Time;
	else if (dt > 0.5f) CurrTime = Time - 0.5f;

	while (CurrTime < Time)
	{
		float Error = n_angulardistance(State, Goal);
		if (n_fabs(Error) > TINY)
		{
			State = n_normangle(State - (Error * Gain * Step));
			CurrTime += Step;
		}
		else
		{
			State = Goal;
			CurrTime = Time;
			break;
		}
	}
}
//---------------------------------------------------------------------

#endif
