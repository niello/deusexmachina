// geometry.hpp	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some basic geometric types.


#ifndef GEOMETRY_H
#define GEOMETRY_H


#include <engine/utility.h>


class	vec3
// 3-element vector class, for 3D math.
{
public:
	vec3() {}
	vec3(float _X, float _Y, float _Z) { x = _X; y = _Y; z = _Z; }
	vec3(const vec3& v) { x = v.x; y = v.y; z = v.z; }

	operator	const float*() const { return &x; }
			
	float	get(int element) const { return (&x)[element]; }
	void	set(int element, float NewValue) { (&x)[element] = NewValue; }
	float	get_x() const { return x; }
	float	get_y() const { return y; }
	float	get_z() const { return z; }
//	float&	x() { return m[0]; }
//	float&	y() { return m[1]; }
//	float&	z() { return m[2]; }
	void	set_xyz(float newx, float newy, float newz) { x = newx; y = newy; z = newz; }
	
	vec3	operator+(const vec3& v) const;
	vec3	operator-(const vec3& v) const;
	vec3	operator-() const;
	float	operator*(const vec3& v) const;
	vec3	operator*(float f) const;
	vec3	operator/(float f) const { return this->operator*(1.0f / f); }
	vec3	cross(const vec3& v) const;

	vec3&	normalize();
	vec3&	operator=(const vec3& v) { x = v.x; y = v.y; z = v.z; return *this; }
	vec3&	operator+=(const vec3& v);
	vec3& operator-=(const vec3& v);
	vec3&	operator*=(float f);
	vec3&	operator/=(float f) { return this->operator*=(1.0f / f); }

	float	magnitude() const;
	float	sqrmag() const;
//	float	min() const;
//	float	max() const;
//	float	minabs() const;
//	float	maxabs() const;

	// Serialize(archive* a);	// archive contains a flag that indicates whether we're reading or writing.
	void	read(SDL_RWops* in);
	void	write(SDL_RWops* out);

	bool	checknan() const;	// Returns true if any component is nan.

	// Some handy vector constants.
	const static vec3	zero, x_axis, y_axis, z_axis;

	float	x, y, z;
//private:
//	float	m[3];
};


#define INLINE_VEC3


#ifdef INLINE_VEC3


inline float	vec3::operator*(const vec3& v) const
// Dot product.
{
	float	result;
	result = x * v.x;
	result += y * v.y;
	result += z * v.z;
	return result;
}


inline vec3&	vec3::operator+=(const vec3& v)
// Adds a vec3 to *this.
{
	x += v.x;
	y += v.y;
	z += v.z;
	return *this;
}


inline vec3&	vec3::operator-=(const vec3& v)
// Subtracts a vec3 from *this.
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
	return *this;
}


#endif // INLINE_VEC3





class	quaternion;


class	matrix
// 3x4 matrix class, for 3D transformations.
{
public:
	matrix() { Identity(); }

	void	Identity();
	void	View(const vec3& ViewNormal, const vec3& ViewUp, const vec3& ViewLocation);
	void	Orient(const vec3& ObjectDirection, const vec3& ObjectUp, const vec3& ObjectLocation);

	static void	Compose(matrix* dest, const matrix& left, const matrix& right);
	vec3	operator*(const vec3& v) const;
	matrix	operator*(const matrix& m) const;
//	operator*=(const quaternion& q);

	matrix&	operator*=(float f);
	matrix&	operator+=(const matrix& m);
	
	void	Invert();
	void	InvertRotation();
	void	NormalizeRotation();
	void	Apply(vec3* result, const vec3& v) const;
	void	ApplyRotation(vec3* result, const vec3& v) const;
	void	ApplyInverse(vec3* result, const vec3& v) const;
	void	ApplyInverseRotation(vec3* result, const vec3& v) const;
	void	Translate(const vec3& v);
	void	SetOrientation(const quaternion& q);
	quaternion	GetOrientation() const;
	
	void	SetColumn(int column, const vec3& v) { m[column] = v; }
	const vec3&	GetColumn(int column) const { return m[column]; }
private:
	vec3	m[4];
};


// class quaternion -- handy for representing rotations.

class quaternion {
public:
	quaternion() : S(1), V(vec3::zero) {}
	quaternion(const quaternion& q) : S(q.S), V(q.V) {}
	quaternion(float s, const vec3& v) : S(s), V(v) {}

	quaternion(const vec3& Axis, float Angle);	// Slightly dubious: semantics varies from other constructor depending on order of arg types.

	float	GetS() const { return S; }
	const vec3&	GetV() const { return V; }
	void	SetS(float s) { S = s; }
	void	SetV(const vec3& v) { V = v; }

	float	get(int i) const { if (i==0) return GetS(); else return V.get(i-1); }
	void	set(int i, float f) { if (i==0) S = f; else V.set(i-1, f); }

	quaternion	operator*(const quaternion& q) const;
	quaternion&	operator*=(float f) { S *= f; V *= f; return *this; }
	quaternion&	operator+=(const quaternion& q) { S += q.S; V += q.V; return *this; }

	quaternion&	operator=(const quaternion& q) { S = q.S; V = q.V; return *this; }
	quaternion&	normalize();
	quaternion&	operator*=(const quaternion& q);
	void	ApplyRotation(vec3* result, const vec3& v);
	
	quaternion	lerp(const quaternion& q, float f) const;
private:
	float	S;
	vec3	V;
};


struct plane_info {
	vec3	normal;
	float	d;

	plane_info() { }

	plane_info( const vec3& n, float dist ) {
		set( n, dist );
	}

	void	set( const vec3& n, float dist ) {
		normal = n;
		d = dist;
	}

	void	set(float nx, float ny, float nz, float dist) {
		normal.set_xyz(nx, ny, nz);
		d = dist;
	}
};


struct collision_info {
	vec3	point;
	vec3	normal;
	// float	distance, or ray_parameter;
};


namespace Geometry {
	vec3	Rotate(float Angle, const vec3& Axis, const vec3& Point);
};



#endif // GEOMETRY_H
