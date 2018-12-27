#ifndef N_PIDFEEDBACKLOOP_H
#define N_PIDFEEDBACKLOOP_H

#include <StdDEM.h>

// A proportional integral derivative feedback loop
// (C) 2006 RadonLabs GmbH

class CPFLoopIntDrv
{
private:

	double value;               // current value of the controller

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

public:

	double goal;            // the value the controller is trying to achieve

	CPFLoopIntDrv();

	void	SetValue(double NewValue);
	void	SetConstants(double PropConst, double IntConst, double DrvConst, double acceleration = 0.0);
	double	GetValue() const { return value; }
	double	GetLastError() const { return lastError; }
	double	GetLastErrorDelta() const { return lastDelta; }

	void	Update(CTime time);
	void	ResetError() { runningError = 0.0f; validError = false; }
};

inline CPFLoopIntDrv::CPFLoopIntDrv() :
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
//---------------------------------------------------------------------

inline void CPFLoopIntDrv::SetValue(double NewValue)
{
	value = NewValue;
	lastError = 0.0;
	lastDelta = 0.0;
}
//---------------------------------------------------------------------

inline void CPFLoopIntDrv::SetConstants(double PropConst, double IntConst, double DrvConst, double acceleration)
{
	pConst = PropConst;
	iConst = IntConst;
	dConst = DrvConst;
	maxAcceleration = acceleration;
}
//---------------------------------------------------------------------

inline void CPFLoopIntDrv::Update(CTime time)
{
	// if too much time has passed, do nothing
	if (time != 0.0f)
	{
		if (time > maxAllowableDeltaTime) time = maxAllowableDeltaTime;

		// compute the error and sum of the errors for the integral
		double error = (goal - value) * time;
		runningError += error;

		// proportional, integral, derivative
		double dP = pConst * error;
		double dI = iConst * runningError * time;
		double dD(0.0f);
		if (validError) dD = dConst * (lastError - error) * time;
		else validError = true;

		// remember the error for derivative
		lastError = error;

		// compute the adjustment
		double thisDelta = dP + dI + dD;

		// clamp the acceleration
		if (maxAcceleration != 0.0f || false)
		{
			double timeRatio(1.0);
			if (lastDeltaTime != 0.0) timeRatio = time / lastDeltaTime;
			lastDeltaTime = time;

			lastDelta *= timeRatio;
			double difference = (thisDelta - lastDelta);
			double accl = maxAcceleration * time * time;

			if (difference < -accl) thisDelta = lastDelta - accl;
			else if (difference > accl) thisDelta = lastDelta + accl;
		}

		// modify the value
		value += thisDelta;
		lastDelta = thisDelta;
	}
}
//---------------------------------------------------------------------

#endif