#pragma once
#ifndef __DEM_L1_MATH_POLAR_H__
#define __DEM_L1_MATH_POLAR_H__

#include <Math/Vector3.h>

// A polar coordinate inline class, consisting of 2 angles Theta (latitude) and Phi (longitude).
// Also offers conversion between cartesian and polar space.
// Allowed range for Theta is 0..180 degree (in rad!) and for Phi 0..360 degree (in rad).
// (C) 2004 RadonLabs GmbH

class CPolar
{
public:

	float Theta;
	float Phi;

	CPolar(): Theta(0.f), Phi(0.f) {}
	CPolar(float ThetaRad, float PhiRad): Theta(ThetaRad), Phi(PhiRad) {}
	CPolar(const vector3& Look) { Set(Look); }
	CPolar(const CPolar& Other): Theta(Other.Theta), Phi(Other.Phi) {}

	vector3	GetCartesianY() const;
	vector3	GetCartesianZ() const;
	void	Set(const CPolar& p) { Theta = p.Theta; Phi = p.Phi; }
	void	Set(float t, float r) { Theta = t; Phi = r; }
	void	Set(const vector3& vec);
	bool	IsEqual(const CPolar& Other, float Tolerance) const;

	CPolar& operator =(const CPolar& Other) { Theta = Other.Theta; Phi = Other.Phi; return *this; }
};

inline void CPolar::Set(const vector3& vec)
{
	vector3 LookNorm(vec);
	LookNorm.norm();

	Theta = acos(LookNorm.y);

	vector2 XZNorm(LookNorm.x, LookNorm.z);
	XZNorm.norm();

	// Adjust Phi based on the quadrant we are in (1, 2, 3, 4)
	if (XZNorm.x >= 0.f && XZNorm.y >= 0.f) Phi = asin(XZNorm.x);
	else if (XZNorm.x < 0.f && XZNorm.y >= 0.f) Phi = asin(XZNorm.y) + PI + HALF_PI;
	else if (XZNorm.x < 0.f && XZNorm.y < 0.f) Phi = asin(-XZNorm.x) + PI;
	else Phi = asin(-XZNorm.y) + HALF_PI;
}
//---------------------------------------------------------------------

// Rotate by Theta around x (inclination), than by Phi around y (azimuth), then get y axis
inline vector3 CPolar::GetCartesianY() const
{
	float sin_theta, cos_theta, sin_rho, cos_rho;
	n_sincos(Theta, sin_theta, cos_theta);
	n_sincos(Phi, sin_rho, cos_rho);
	return vector3(sin_theta * sin_rho, cos_theta, -sin_theta * cos_rho);
}
//---------------------------------------------------------------------

// Rotate by Theta around x (inclination), than by Phi around y (azimuth), then get z axis
inline vector3 CPolar::GetCartesianZ() const
{
	float sin_theta, cos_theta, sin_rho, cos_rho;
	n_sincos(Theta, sin_theta, cos_theta);
	n_sincos(Phi, sin_rho, cos_rho);
	return vector3(-sin_rho * cos_theta, sin_theta, cos_theta * cos_rho);
}
//---------------------------------------------------------------------

inline bool CPolar::IsEqual(const CPolar& Other, float Tolerance) const
{
	return n_fequal(Other.Theta, Theta, Tolerance) && n_fequal(Other.Phi, Phi, Tolerance);
}
//---------------------------------------------------------------------

#endif
