#ifndef N_PQUATFEEDBACKLOOP_H
#define N_PQUATFEEDBACKLOOP_H

#include <mathlib/quaternion.h>

// A specialized proportional feedback loop for quaternion rotations
// (C) 2004 RadonLabs GmbH

class CPFLoopQuat
{
private:

	float		CurrTime;
	float		Step;

public:

	float		Gain;
	quaternion	Goal;
	quaternion	State;

	CPFLoopQuat(): CurrTime(0.f), Step(0.001f), Gain(-1.f) {}

	void Reset(float Time, float StepSize) { CurrTime = Time; Step = StepSize; }
	void Update(float CurrTime);
};

inline void CPFLoopQuat::Update(float Time)
{
	float dt = Time - CurrTime;
	if (dt < 0.f) CurrTime = Time;
	else if (dt > 0.5f) CurrTime = Time - 0.5f;

	while (CurrTime < Time)
	{
		// Compute angular Error between State and Goal
		vector3 stateVec = State.rotate(vector3::AxisZ);
		vector3 goalVec = Goal.rotate(vector3::AxisZ);

		// Dot is between -1 (opposite vectors) and 1 (identical vectors),
		// generate a lerp value between 0 (represents Goal) and 1 (represents State)
		float Error = (-(stateVec % goalVec) + 1.0f) * 0.5f;

		quaternion res;
		res.slerp(State, Goal, Error * Gain * Step);
		State = res;

		CurrTime += Step;
	}
}
//---------------------------------------------------------------------

#endif