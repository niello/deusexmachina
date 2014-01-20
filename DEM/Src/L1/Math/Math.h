#pragma once
#ifndef __DEM_L1_MATH_H__
#define __DEM_L1_MATH_H__

#include <StdDEM.h>
#include <mathlib/nmath.h>

// Declarations and utility functions

//???different min, max and clamp functions here?
//move nmath.h here

namespace Math
{

// Solves ax^2 + bx + c = 0 equation. Returns a number of real roots and optionally root values.
inline DWORD SolveQuadraticEquation(float a, float b, float c, float* pOutX1 = NULL, float* pOutX2 = NULL)
{
	float D = b * b - 4.f * a * c;

	if (D < 0.f) return 0;

	if (D == 0.f)
	{
		if (pOutX1) *pOutX1 = -b / (2.f * a);
		return 1;
	}

	float SqrtD = n_sqrt(D);
	float InvDenom = 0.5f / a;
	if (pOutX1) *pOutX1 = (-b - SqrtD) * InvDenom;
	if (pOutX2) *pOutX2 = (SqrtD - b) * InvDenom;
	return 2;
}
//---------------------------------------------------------------------

}

#endif
