#ifndef N_LINE_H
#define N_LINE_H

#include <mathlib/vector.h>

// A 3D line class
// (C) 2004 RadonLabs GmbH

class line3
{
public:

	vector3 Start;
	vector3 Vector;

	line3() {}
	line3(const vector3& v0, const vector3& v1): Start(v0), Vector(v1 - v0) {}
	line3(const line3& Other): Start(Other.Start), Vector(Other.Vector) {}

	void	set(const vector3& v0, const vector3& v1) { Start = v0; Vector = v1 - v0; }

	float	distance(const vector3& Pt) const;
	bool	intersect(const line3& l, vector3& pa, vector3& pb) const;
	float	closestpoint(const vector3& Pt) const;

	vector3	End() const { return Start + Vector; }
	float	len() const { return Vector.len(); }
	float	lensquared() const { return Vector.lensquared(); }
	vector3	ipol(float t) const { return Start + Vector * t; }
};

// Returns a point on the line which is closest to a another point in space.
// This just returns the parameter t on where the point is located. If t is
// between 0 and 1, the point is on the line, otherwise not. Actual 3d point:
// Pt = Vector + Start*t
inline float line3::closestpoint(const vector3& Pt) const
{
	float l = (Vector % Vector);
	return (l > 0.0f) ? (Vector % (Pt - Start)) / l : 0.0f;
}
//---------------------------------------------------------------------

inline float line3::distance(const vector3& Pt) const
{
	vector3 Diff(Pt - Start);
	float l = (Vector % Vector);
	if (l > 0.0f) Diff = Diff - Vector * (Vector % Diff) / l;
	return Diff.len();
}
//---------------------------------------------------------------------

// Get line/line intersection. Returns the shortest line between two lines.
inline bool line3::intersect(const line3& l, vector3& pa, vector3& pb) const
{
	const float EPS = 2.22e-16f;
	vector3 p1 = Start;
	vector3 p2 = ipol(10.0f);
	vector3 p3 = l.Start;
	vector3 p4 = l.ipol(10.0f);
	vector3 p13, p43, p21;
	float d1343,d4321,d1321,d4343,d2121;
	float numer, denom;
	float mua, mub;

	p13.x = p1.x - p3.x;
	p13.y = p1.y - p3.y;
	p13.z = p1.z - p3.z;
	p43.x = p4.x - p3.x;
	p43.y = p4.y - p3.y;
	p43.z = p4.z - p3.z;
	if (n_abs(p43.x) < EPS && n_abs(p43.y) < EPS && n_abs(p43.z) < EPS) return false;
	p21.x = p2.x - p1.x;
	p21.y = p2.y - p1.y;
	p21.z = p2.z - p1.z;
	if (n_abs(p21.x)  < EPS && n_abs(p21.y) < EPS && n_abs(p21.z) < EPS) return false;

	d1343 = p13.x * p43.x + p13.y * p43.y + p13.z * p43.z;
	d4321 = p43.x * p21.x + p43.y * p21.y + p43.z * p21.z;
	d1321 = p13.x * p21.x + p13.y * p21.y + p13.z * p21.z;
	d4343 = p43.x * p43.x + p43.y * p43.y + p43.z * p43.z;
	d2121 = p21.x * p21.x + p21.y * p21.y + p21.z * p21.z;

	denom = d2121 * d4343 - d4321 * d4321;
	if (n_abs(denom) < EPS) return false;
	numer = d1343 * d4321 - d1321 * d4343;

	mua = numer / denom;
	mub = (d1343 + d4321 * (mua)) / d4343;

	pa.x = p1.x + mua * p21.x;
	pa.y = p1.y + mua * p21.y;
	pa.z = p1.z + mua * p21.z;
	pb.x = p3.x + mub * p43.x;
	pb.y = p3.y + mub * p43.y;
	pb.z = p3.z + mub * p43.z;

	return true;
}
//---------------------------------------------------------------------

#endif
