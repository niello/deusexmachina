#ifndef N_POLAR_H
#define N_POLAR_H
//------------------------------------------------------------------------------
/**
    @class polar2
    @ingroup NebulaMathDataTypes

    A polar coordinate inline class, consisting of 2 angles theta (latitude)
    and phi (longitude). Also offers conversion between cartesian and
    polar space.

    Allowed range for theta is 0..180 degree (in rad!) and for phi 0..360 degree
    (in rad).

    (C) 2004 RadonLabs GmbH
*/
#include "mathlib/vector.h"

class polar2
{
private:

	// the equal operator is not allowed, use isequal() with tolerance!
	bool operator==(const polar2& /*rhs*/) { return false; }

public:

	float theta;
	float phi;

	polar2(): theta(0.0f), phi(0.0f) {}
	polar2(float t, float r): theta(t), phi(r) {}
	polar2(const vector3& v) { set(v); }
	polar2(const polar2& src): theta(src.theta), phi(src.phi) {}

	polar2& operator =(const polar2& rhs);

	vector3	get_cartesian_y() const;
	vector3	get_cartesian_z() const;
	//matrix33 get_matrix() const;
	void	set(const polar2& p) { theta = p.theta; phi = p.phi; }
	void	set(float t, float r) { theta = t; phi = r; }
	void	set(const vector3& vec);
	bool	isequal(const polar2& rhs, float tol) const;
};

inline polar2& polar2::operator=(const polar2& rhs)
{
	theta = rhs.theta;
	phi = rhs.phi;
	return *this;
}
//---------------------------------------------------------------------

inline void polar2::set(const vector3& vec)
{
    vector3 v3(vec);
    v3.norm();

    double dTheta = acos(v3.y);

    // build a normalized 2d vector of the xz component
    vector2 v2(v3.x, v3.z);
    v2.norm();

    // adjust dRho based on the quadrant we are in
    double dRho;
    if ((v2.x >= 0.0f) && (v2.y >= 0.0f))
    {
        // quadrant 1
        dRho = asin(v2.x);
    }
    else if ((v2.x < 0.0f) && (v2.y >= 0.0f))
    {
        // quadrant 2
        dRho = asin(v2.y) + n_deg2rad(270.0f);
    }
    else if ((v2.x < 0.0f) && (v2.y < 0.0f))
    {
        // quadrant 3
        dRho = asin(-v2.x) + n_deg2rad(180.0f);
    }
    else
    {
        // quadrant 4
        dRho = asin(-v2.y) + n_deg2rad(90.0f);
    }

    theta = (float) dTheta;
    phi   = (float) dRho;
}
//---------------------------------------------------------------------

// Rotate by theta around x (inclination), than by phi around y (azimuth), then get y axis
inline vector3 polar2::get_cartesian_y() const
{
	float sin_theta, cos_theta, sin_rho, cos_rho;
	n_sincos(theta, sin_theta, cos_theta);
	n_sincos(phi, sin_rho, cos_rho);
	return vector3(sin_theta * sin_rho, cos_theta, -sin_theta * cos_rho);
}
//---------------------------------------------------------------------

// Rotate by theta around x (inclination), than by phi around y (azimuth), then get z axis
inline vector3 polar2::get_cartesian_z() const
{
	float sin_theta, cos_theta, sin_rho, cos_rho;
	n_sincos(theta, sin_theta, cos_theta);
	n_sincos(phi, sin_rho, cos_rho);
	return vector3(-sin_rho * cos_theta, sin_theta, cos_theta * cos_rho);
}
//---------------------------------------------------------------------

inline bool polar2::isequal(const polar2& rhs, float tol) const
{
	return (n_abs(rhs.theta - theta) <= tol) && (n_abs(rhs.phi - phi) <= tol);
}
//---------------------------------------------------------------------

#endif
