#pragma once
#ifndef __DEM_L1_MATH_AABB_H__
#define __DEM_L1_MATH_AABB_H__

#include <Math/Matrix44.h>
#include <Math/plane.h>

// An axis-aligned bounding box class

class CAABB
{
public:

	enum
	{
		OUTSIDE     = 0,
		ISEQUAL     = (1<<0),
		ISCONTAINED = (1<<1),
		CONTAINS    = (1<<2),
		CLIPS       = (1<<3),
	};

	vector3 Min;
	vector3 Max;

	CAABB() {}
	CAABB(const vector3& Center, const vector3& Extents): Min(Center - Extents), Max(Center + Extents) {}
	CAABB(const matrix44& m) { Set(m); }

	vector3		Center() const { return (Min + Max) * 0.5f; }
	vector3		Extents() const { return (Max - Min) * 0.5f; }
	vector3		Size() const { return Max - Min; }
	float		GetDiagonalLength() const { return vector3::Distance(Min, Max); }

	void		Set(const matrix44& m);
	void		Set(const vector3& Center, const vector3& Extents) { Min = Center - Extents; Max = Center + Extents; }

	void		BeginExtend();
	void		BeginExtend(const vector3& InitialPoint);
	void		Extend(float x, float y, float z);
	void		ExtendFast(float x, float y, float z);
	void		Extend(const vector3& v) { Extend(v.x, v.y, v.z); }
	void		ExtendFast(const vector3& v) { ExtendFast(v.x, v.y, v.z); }
	void		Extend(const CAABB& box);
	void		EndExtend();

	void		Transform(const matrix44& m);
	void		Transform(const matrix44& m, CAABB& Out) const;
	void		TransformDivW(const matrix44& m);
	void		TransformDivW(const matrix44& m, CAABB& Out) const;
	void		ToMatrix44(matrix44& Out) const;
	vector3		GetCorner(DWORD index) const;
	void		GetClipPlanes(const matrix44& ViewProj, plane outPlanes[6]) const;

	bool		contains(const CAABB& box) const;
	bool		contains(const vector3& pos) const;
	EClipStatus	GetClipStatus(const CAABB& other) const;
	EClipStatus	GetClipStatus(const matrix44& ViewProj) const;
	bool		intersects(const CAABB& box) const;
	int			line_test(float v0, float v1, float w0, float w1);
	int			intersect(const CAABB& box);
	bool		intersect(const line3& line, vector3& ipos) const;
	bool		isect_const_x(const float x, const line3& l, vector3& out) const;
	bool		isect_const_y(const float y, const line3& l, vector3& out) const;
	bool		isect_const_z(const float z, const line3& l, vector3& out) const;
	bool		PointInPolyX(const vector3& p) const { return p.y >= Min.y && p.y <= Max.y && p.z >= Min.z && p.z <= Max.z; }
	bool		PointInPolyY(const vector3& p) const { return p.x >= Min.x && p.x <= Max.x && p.z >= Min.z && p.z <= Max.z; }
	bool		PointInPolyZ(const vector3& p) const { return p.x >= Min.x && p.x <= Max.x && p.y >= Min.y && p.y <= Max.y; }
};

// Construct a bounding box around a 4x4 matrix. The translational part defines the
// Center point, and the x,y,z vectors of the matrix define the Extents.
inline void CAABB::Set(const matrix44& m)
{
	float xExtent = n_max(n_max(n_fabs(m.M11), n_fabs(m.M21)), n_fabs(m.M31));
	float yExtent = n_max(n_max(n_fabs(m.M12), n_fabs(m.M22)), n_fabs(m.M32));
	float zExtent = n_max(n_max(n_fabs(m.M13), n_fabs(m.M23)), n_fabs(m.M33));
	vector3 extent(xExtent, yExtent, zExtent);
	vector3 Center = m.Translation();
	Min = Center - extent;
	Max = Center + extent;
}
//---------------------------------------------------------------------

inline void CAABB::BeginExtend()
{
	Min.set(FLT_MAX, FLT_MAX, FLT_MAX);
	Max.set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
}
//---------------------------------------------------------------------

inline void CAABB::BeginExtend(const vector3& InitialPoint)
{
	Min = InitialPoint;
	Max = InitialPoint;
}
//---------------------------------------------------------------------

inline void CAABB::EndExtend()
{
	if (Min.x > Max.x || Min.y > Max.y || Min.z > Max.z)
	{
		Min = vector3::Zero;
		Max = vector3::Zero;
	}
}
//---------------------------------------------------------------------

inline void CAABB::Extend(float x, float y, float z)
{
	if (x < Min.x) Min.x = x;
	if (x > Max.x) Max.x = x;
	if (y < Min.y) Min.y = y;
	if (y > Max.y) Max.y = y;
	if (z < Min.z) Min.z = z;
	if (z > Max.z) Max.z = z;
}
//---------------------------------------------------------------------

// Requires Min <= Max
inline void CAABB::ExtendFast(float x, float y, float z)
{
	if (x < Min.x) Min.x = x;
	else if (x > Max.x) Max.x = x;
	if (y < Min.y) Min.y = y;
	else if (y > Max.y) Max.y = y;
	if (z < Min.z) Min.z = z;
	else if (z > Max.z) Max.z = z;
}
//---------------------------------------------------------------------

inline void CAABB::Extend(const CAABB& box)
{
	if (box.Min.x < Min.x) Min.x = box.Min.x;
	if (box.Min.y < Min.y) Min.y = box.Min.y;
	if (box.Min.z < Min.z) Min.z = box.Min.z;
	if (box.Max.x > Max.x) Max.x = box.Max.x;
	if (box.Max.y > Max.y) Max.y = box.Max.y;
	if (box.Max.z > Max.z) Max.z = box.Max.z;
}
//---------------------------------------------------------------------

inline vector3 CAABB::GetCorner(DWORD index) const
{
	n_assert_dbg(index < 8);
	switch (index)
	{
		case 0:		return Min;
		case 1:		return vector3(Min.x, Max.y, Min.z);
		case 2:		return vector3(Max.x, Max.y, Min.z);
		case 3:		return vector3(Max.x, Min.y, Min.z);
		case 4:		return Max;
		case 5:		return vector3(Min.x, Max.y, Max.z);
		case 6:		return vector3(Min.x, Min.y, Max.z);
		default:	return vector3(Max.x, Min.y, Max.z);
	}
}
//---------------------------------------------------------------------

// Get the bounding box's side planes in clip space.
inline void CAABB::GetClipPlanes(const matrix44& viewProj, plane outPlanes[6]) const
{
	matrix44 inv = viewProj;
	inv.invert();
	inv.transpose();

	vector4 planes[6];
	planes[0].set(-1, 0, 0, +Max.x);
	planes[1].set(+1, 0, 0, -Min.x);
	planes[2].set(0, -1, 0, +Max.y);
	planes[3].set(0, +1, 0, -Min.y);
	planes[4].set(0, 0, -1, +Max.z);
	planes[5].set(0, 0, +1, -Min.z);

	for (int i = 0; i < 6; ++i)
	{
		vector4 v = inv * planes[i];
		outPlanes[i].set(v.x, v.y, v.z, v.w);
	}
}
//---------------------------------------------------------------------

// Transforms this axis aligned bounding by the 4x4 matrix. This bounding
// box must be axis aligned with the matrix, the resulting bounding
// will be axis aligned in the matrix' "destination" space.
inline void CAABB::Transform(const matrix44& m)
{
	/*  ?? BUG ??
	// Extent the matrix' (x,y,z) components by our own extent vector.
	vector3 Extents = Extents();
	vector3 Center  = Center();
	matrix44 extentMatrix(
		m.M11 * Extents.x, m.M12 * Extents.x, m.M13 * Extents.x, 0.0f,
		m.M21 * Extents.y, m.M22 * Extents.y, m.M23 * Extents.y, 0.0f,
		m.M31 * Extents.z, m.M32 * Extents.z, m.M33 * Extents.z, 0.0f,
		m.M41 + Center.x,  m.M42 + Center.y,  m.M43 + Center.z,  1.0f);
	Set(extentMatrix);
	*/

	vector3 NewMin = m * GetCorner(0);
	vector3 NewMax = Min;
	for (int i = 1; i < 8; ++i)
	{
		vector3 OBBCorner = m * GetCorner(i);
		if (OBBCorner.x < NewMin.x) NewMin.x = OBBCorner.x;
		else if (OBBCorner.x > NewMax.x) NewMax.x = OBBCorner.x;
		if (OBBCorner.y < NewMin.y) NewMin.y = OBBCorner.y;
		else if (OBBCorner.y > NewMax.y) NewMax.y = OBBCorner.y;
		if (OBBCorner.z < NewMin.z) NewMin.z = OBBCorner.z;
		else if (OBBCorner.z > NewMax.z) NewMax.z = OBBCorner.z;
	}

	Min = NewMin;
	Max = NewMax;
}
//---------------------------------------------------------------------

inline void CAABB::Transform(const matrix44& m, CAABB& Out) const
{
	Out.BeginExtend(m * GetCorner(0));
	for (int i = 1; i < 8; ++i) Out.ExtendFast(m * GetCorner(i));
}
//---------------------------------------------------------------------

// Same as Transform() but does a div-by-w on the way (useful for transforming to screen space)
inline void CAABB::TransformDivW(const matrix44& m)
{
	vector3 Min = m.mult_divw(GetCorner(0));
	vector3 Max = Min;
	for (int i = 1; i < 8; ++i)
	{
		vector3 OBBCorner = m.mult_divw(GetCorner(i));
		if (OBBCorner.x < Min.x) Min.x = OBBCorner.x;
		else if (OBBCorner.x > Max.x) Max.x = OBBCorner.x;
		if (OBBCorner.y < Min.y) Min.y = OBBCorner.y;
		else if (OBBCorner.y > Max.y) Max.y = OBBCorner.y;
		if (OBBCorner.z < Min.z) Min.z = OBBCorner.z;
		else if (OBBCorner.z > Max.z) Max.z = OBBCorner.z;
	}

	Min = Min;
	Max = Max;
}
//---------------------------------------------------------------------

inline void CAABB::TransformDivW(const matrix44& m, CAABB& Out) const
{
	Out.BeginExtend(m.mult_divw(GetCorner(0)));
	for (int i = 1; i < 8; ++i) Out.ExtendFast(m.mult_divw(GetCorner(i)));
}
//---------------------------------------------------------------------

// Check for intersection of 2 axis aligned bounding boxes in the same coordinate space
inline bool CAABB::intersects(const CAABB& box) const
{
	return	Max.x >= box.Min.x && Min.x <= box.Max.x &&
			Max.y >= box.Min.y && Min.y <= box.Max.y &&
			Max.z >= box.Min.z && Min.z <= box.Max.z;
}
//---------------------------------------------------------------------

// Check if the other box is completely contained in this bounding box
inline bool CAABB::contains(const CAABB& box) const
{
	return	Min.x <= box.Min.x && Max.x >= box.Max.x &&
			Min.y <= box.Min.y && Max.y >= box.Max.y &&
			Min.z <= box.Min.z && Max.z >= box.Max.z;
}
//---------------------------------------------------------------------

// Check if position is inside bounding box.
inline bool CAABB::contains(const vector3& v) const
{
	return	Min.x <= v.x && Max.x >= v.x &&
			Min.y <= v.y && Max.y >= v.y &&
			Min.z <= v.z && Max.z >= v.z;
}
//---------------------------------------------------------------------

inline EClipStatus CAABB::GetClipStatus(const CAABB& other) const
{
	if (contains(other)) return Inside;
	if (intersects(other)) return Clipped;
	return Outside;
}
//---------------------------------------------------------------------

// Check for intersection with a view volume defined by a view-projection matrix
inline EClipStatus CAABB::GetClipStatus(const matrix44& ViewProj) const
{
	int ANDFlags = 0xffff;
	int ORFlags = 0;
	for (int i = 0; i < 8; ++i)
	{
		vector4 CornerProj = ViewProj * vector4(GetCorner(i));
		int ClipBits = 0;
		if (CornerProj.x < -CornerProj.w)		ClipBits |= (1 << 0);	// Left
		else if (CornerProj.x > CornerProj.w)	ClipBits |= (1 << 1);	// Right
		if (CornerProj.y < -CornerProj.w)		ClipBits |= (1 << 2);	// Bottom
		else if (CornerProj.y > CornerProj.w)	ClipBits |= (1 << 3);	// Top
		if (CornerProj.z < -CornerProj.w)		ClipBits |= (1 << 4);	// Far
		else if (CornerProj.z > CornerProj.w)	ClipBits |= (1 << 5);	// Near
		ANDFlags &= ClipBits;
		ORFlags |= ClipBits;
	}
	if (!ORFlags) return Inside;
	if (ANDFlags) return Outside;
	return Clipped;
}
//---------------------------------------------------------------------

// Create a transform matrix which transforms an unit cube to this box
inline void CAABB::ToMatrix44(matrix44& Out) const
{
    Out.set(Max.x - Min.x, 0.f, 0.f, 0.f,
			0.f, Max.y - Min.y, 0.f, 0.f,
			0.f, 0.f, Max.z - Min.z, 0.f,
			(Min.x + Max.x) * 0.5f, (Min.y + Max.y) * 0.5f, (Min.z + Max.z) * 0.5f, 1.f);
}
//---------------------------------------------------------------------

// Get point of intersection of 3d line with planes on const x,y,z
inline bool CAABB::isect_const_x(const float x, const line3& l, vector3& out) const
{
	if (l.Vector.x == 0.f) FAIL;
	float t = (x - l.Start.x) / l.Vector.x;
	if (t < 0.f || t > 1.f) FAIL;
	out = l.ipol(t);
	OK;
}
//---------------------------------------------------------------------

inline bool CAABB::isect_const_y(const float y, const line3& l, vector3& out) const
{
	if (l.Vector.y == 0.f) FAIL;
	float t = (y - l.Start.y) / l.Vector.y;
	if (t < 0.f || t > 1.f) FAIL;
	out = l.ipol(t);
	OK;
}
//---------------------------------------------------------------------

inline bool CAABB::isect_const_z(const float z, const line3& l, vector3& out) const
{
	if (l.Vector.z == 0.f) FAIL;
	float t = (z - l.Start.z) / l.Vector.z;
	if (t < 0.f || t > 1.f) FAIL;
	out = l.ipol(t);
	OK;
}
//---------------------------------------------------------------------

/**
    @brief Gets closest intersection with AABB.
    If the line starts inside the box, start point is returned in ipos.
    @param line the pick ray
    @param ipos closest point of intersection if successful, trash otherwise
    @return true if an intersection occurs
*/
inline bool CAABB::intersect(const line3& line, vector3& ipos) const
{
    // Handle special case for start point inside box
    if (line.Start.x >= Min.x && line.Start.y >= Min.y && line.Start.z >= Min.z &&
        line.Start.x <= Max.x && line.Start.y <= Max.y && line.Start.z <= Max.z)
    {
        ipos = line.Start;
        return true;
    }

    // Order planes to check, closest three only
    int plane[3];
	plane[0] = line.Vector.x > 0 ? 0 : 1;
	plane[1] = line.Vector.y > 0 ? 2 : 3;
	plane[2] = line.Vector.z > 0 ? 4 : 5;

    for (int i = 0; i < 3; ++i)
    {
        switch (plane[i])
        {
            case 0:
                if (isect_const_x(Min.x,line,ipos) && PointInPolyX(ipos)) return true;
                break;
            case 1:
                if (isect_const_x(Max.x,line,ipos) && PointInPolyX(ipos)) return true;
                break;
            case 2:
                if (isect_const_y(Min.y,line,ipos) && PointInPolyY(ipos)) return true;
                break;
            case 3:
                if (isect_const_y(Max.y,line,ipos) && PointInPolyY(ipos)) return true;
                break;
            case 4:
                if (isect_const_z(Min.z,line,ipos) && PointInPolyZ(ipos)) return true;
                break;
            case 5:
                if (isect_const_z(Max.z,line,ipos) && PointInPolyZ(ipos)) return true;
                break;
        }
    }

    return false;
}
//---------------------------------------------------------------------

inline int CAABB::line_test(float v0, float v1, float w0, float w1)
{
	// quick rejection test
	if ((v1 < w0) || (v0 > w1)) return OUTSIDE;
	if ((v0 == w0) && (v1 == w1)) return ISEQUAL;
	if ((v0 >= w0) && (v1 <= w1)) return ISCONTAINED;
	if ((v0 <= w0) && (v1 >= w1)) return CONTAINS;
	return CLIPS;
}
//---------------------------------------------------------------------

// Check if box intersects, contains or is contained in other box by doing 3 projection
// tests for each dimension, if all 3 test return true, then the 2 boxes intersect.
inline int CAABB::intersect(const CAABB& box)
{
	int and_code = 0xffff;
	int or_code = 0;
	int cx = line_test(Min.x, Max.x, box.Min.x, box.Max.x);
	and_code &= cx;
	or_code |= cx;
	int cy = line_test(Min.y, Max.y, box.Min.y, box.Max.y);
	and_code &= cy;
	or_code |= cy;
	int cz = line_test(Min.z, Max.z, box.Min.z, box.Max.z);
	and_code &= cz;
	or_code |= cz;
	if (or_code == 0) return OUTSIDE;
	if (and_code != 0) return and_code;
	if (cx && cy && cz) return CLIPS;
	return OUTSIDE;
}
//---------------------------------------------------------------------

#endif
