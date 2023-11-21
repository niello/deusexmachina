#pragma once
#include <Math/Vector3.h>
#include <Math/Vector2.h>
#include <rtm/vector4f.h>

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

	void	Set(const CPolar& p) { Theta = p.Theta; Phi = p.Phi; }
	void	Set(float t, float r) { Theta = t; Phi = r; }
	void	Set(const vector3& vec);
	bool	IsEqual(const CPolar& Other, float Tolerance) const;

	// Rotate by Theta around x (inclination), than by Phi around y (azimuth), then get y axis
	rtm::vector4f RTM_SIMD_CALL CPolar::GetCartesianY() const
	{
		float SinTheta, CosTheta, SinPhi, CosPhi;
		n_sincos(Theta, SinTheta, CosTheta);
		n_sincos(Phi, SinPhi, CosPhi);
		return rtm::vector_set(SinTheta * SinPhi, CosTheta, -SinTheta * CosPhi);
	}

	// Rotate by Theta around x (inclination), than by Phi around y (azimuth), then get z axis
	rtm::vector4f RTM_SIMD_CALL CPolar::GetCartesianZ() const
	{
		float SinTheta, CosTheta, SinPhi, CosPhi;
		n_sincos(Theta, SinTheta, CosTheta);
		n_sincos(Phi, SinPhi, CosPhi);
		return rtm::vector_set(-SinPhi * CosTheta, SinTheta, CosTheta * CosPhi);
	}

	CPolar& operator =(const CPolar& Other) { Theta = Other.Theta; Phi = Other.Phi; return *this; }
};

inline void CPolar::Set(const vector3& vec)
{
	vector3 LookNorm(vec);
	LookNorm.norm();

	Theta = acosf(LookNorm.y);

	vector2 XZNorm(LookNorm.x, LookNorm.z);
	XZNorm.norm();

	// Adjust Phi based on the quadrant we are in (1, 2, 3, 4)
	if (XZNorm.x >= 0.f && XZNorm.y >= 0.f) Phi = asinf(XZNorm.x);
	else if (XZNorm.x < 0.f && XZNorm.y >= 0.f) Phi = asinf(XZNorm.y) + PI + HALF_PI;
	else if (XZNorm.x < 0.f && XZNorm.y < 0.f) Phi = asinf(-XZNorm.x) + PI;
	else Phi = asinf(-XZNorm.y) + HALF_PI;
}
//---------------------------------------------------------------------

inline bool CPolar::IsEqual(const CPolar& Other, float Tolerance) const
{
	return n_fequal(Other.Theta, Theta, Tolerance) && n_fequal(Other.Phi, Phi, Tolerance);
}
//---------------------------------------------------------------------
