#ifndef N_PLANE_H
#define N_PLANE_H

#include <Math/line.h>

// A plane in 3d space
// (C) 2004 RadonLabs GmbH

class plane
{
public:

	float a , b, c, d; //???vector3 Normal + D? or even internal vector 4 representation?

	plane(): a(0.0f), b(0.0f), c(0.0f), d(1.0f) {}
	plane(float A, float B, float C, float D): a(A), b(B), c(C), d(D) {}
	plane(const plane& Other): a(Other.a), b(Other.b), c(Other.c), d(Other.d) {}
	plane(const vector3& v0, const vector3& v1, const vector3& v2) { set(v0, v1, v2); }

	void	set(float A, float B, float C, float D) { a = A; b = B; c = C; d = D; }
	void	set(const vector3& v0, const vector3& v1, const vector3& v2);
	float	distance(const vector3& v) const { return a * v.x + b * v.y + c * v.z + d; }
	vector3	normal() const { return vector3(a, b, c); }
	bool	intersect(const line3& l, float& t) const;
	bool	intersect(const plane& p, line3& l) const;
};

inline void plane::set(const vector3& v0, const vector3& v1, const vector3& v2)
{
	vector3 cross((v2 - v0) * (v1 - v0));
	cross.norm();
	a = cross.x;
	b = cross.y;
	c = cross.z;
	d = -(a * v0.x + b * v0.y + c * v0.z);
}
//---------------------------------------------------------------------

// if (p.intersect(l, t)) { vector3 intersection = l.Start + t*l.Vector; }
inline bool plane::intersect(const line3& l, float& t) const
{
	float f0 = a * l.Start.x + b * l.Start.y + c * l.Start.z + d;
	float f1 = a * -l.Vector.x + b * -l.Vector.y + c * -l.Vector.z;
	if (f1 >= -0.0001f && f1 <= 0.0001f) return false;
	t = f0 / f1;
	return true;
}
//---------------------------------------------------------------------

inline bool plane::intersect(const plane& p, line3& l) const
{
	vector3 n0 = normal();
	vector3 n1 = p.normal();
	float n00 = n0 % n0;
	float n01 = n0 % n1;
	float n11 = n1 % n1;
	float det = n00 * n11 - n01 * n01;
	const float tol = 1e-06f;
	if (n_fabs(det) < tol) return false;
	float inv_det = 1.0f/det;
	float c0 = (n11 * d - n01 * p.d)    * inv_det;
	float c1 = (n00 * p.d - n01 * d)* inv_det;
	l.Vector = n0 * n1;
	l.Start = n0 * c0 + n1 * c1;
	return true;
}
//---------------------------------------------------------------------

#endif
