#ifndef N_BBOX_H
#define N_BBOX_H
//------------------------------------------------------------------------------
/**
    @class bbox3
    @ingroup NebulaMathDataTypes

    A non-oriented bounding box class.

    (C) 2004 RadonLabs GmbH
*/
#include "mathlib/vector.h"
#include "mathlib/matrix44.h"
#include "mathlib/line.h"
#include "mathlib/plane.h"

//------------------------------------------------------------------------------
//  bbox3
//------------------------------------------------------------------------------
class bbox3
{
public:
    /// clip codes
    enum
    {
        ClipLeft   = (1<<0),
        ClipRight  = (1<<1),
        ClipBottom = (1<<2),
        ClipTop    = (1<<3),
        ClipNear   = (1<<4),
        ClipFar    = (1<<5),
    };

    enum {
        OUTSIDE     = 0,
        ISEQUAL     = (1<<0),
        ISCONTAINED = (1<<1),
        CONTAINS    = (1<<2),
        CLIPS       = (1<<3),
    };

	vector3 vmin;
    vector3 vmax;

	bbox3() {}
	bbox3(const vector3& center, const vector3& extents): vmin(center - extents), vmax(center + extents) {}
	bbox3(const matrix44& m) { set(m); }
 
	vector3 center() const { return (vmin + vmax) * 0.5f; }
	vector3 extents() const { return (vmax - vmin) * 0.5f; }
	vector3 size() const { return vmax - vmin; }
	float GetDiagonalLength() const { return vector3::Distance(vmin, vmax); }

	void set(const matrix44& m);
	void set(const vector3& center, const vector3& extents) { vmin = center - extents; vmax = center + extents; }

	void BeginExtend();
	void BeginExtend(const vector3& InitialPoint);
	void Extend(float x, float y, float z);
	void ExtendFast(float x, float y, float z);
	void Extend(const vector3& v) { Extend(v.x, v.y, v.z); }
	void ExtendFast(const vector3& v) { ExtendFast(v.x, v.y, v.z); }
	void Extend(const bbox3& box);
	void EndExtend();

	void Transform(const matrix44& m);
    /// transform bounding box with divide by w
    void transform_divw(const matrix44& m);
    /// check for intersection with axis aligned bounding box
    bool intersects(const bbox3& box) const;
    /// check if this box completely contains the parameter box
    bool contains(const bbox3& box) const;
    /// return true if this box contains the position
    bool contains(const vector3& pos) const;
    /// check for intersection with other bounding box
    EClipStatus clipstatus(const bbox3& other) const;
    /// check for intersection with projection volume
    EClipStatus clipstatus(const matrix44& ViewProj) const;
    /// create a matrix which transforms an unit cube to this bounding box
    void ToMatrix44(matrix44& Out) const;
    /// return one of the 8 corner points
    vector3 GetCorner(DWORD index) const;
    /// return side planes in clip space
    void get_clipplanes(const matrix44& ViewProj, plane outPlanes[6]) const;

    int line_test(float v0, float v1, float w0, float w1);
    int intersect(const bbox3& box);
    bool intersect(const line3& line, vector3& ipos) const;

    // get point of intersection of 3d line with planes
    // on const x,y,z
    bool isect_const_x(const float x, const line3& l, vector3& out) const
    {
        if (l.Vector.x != 0.0f)
        {
            float t = (x - l.Start.x) / l.Vector.x;
            if ((t >= 0.0f) && (t <= 1.0f))
            {
                // point of intersection...
                out = l.ipol(t);
                return true;
            }
        }
        return false;
    }
    bool isect_const_y(const float y, const line3& l, vector3& out) const
    {
        if (l.Vector.y != 0.0f)
        {
            float t = (y - l.Start.y) / l.Vector.y;
            if ((t >= 0.0f) && (t <= 1.0f))
            {
                // point of intersection...
                out = l.ipol(t);
                return true;
            }
        }
        return false;
    }
    bool isect_const_z(const float z, const line3& l, vector3& out) const
    {
        if (l.Vector.z != 0.0f)
        {
            float t = (z - l.Start.z) / l.Vector.z;
            if ((t >= 0.0f) && (t <= 1.0f))
            {
                // point of intersection...
                out = l.ipol(t);
                return true;
            }
        }
        return false;
    }

    // point in polygon check for sides with constant x,y and z
    bool pip_const_x(const vector3& p) const
    {
        if ((p.y >= vmin.y) && (p.y <= vmax.y) && (p.z >= vmin.z) && (p.z <= vmax.z)) return true;
        return false;
    }
    bool pip_const_y(const vector3& p) const
    {
        if ((p.x >= vmin.x) && (p.x <= vmax.x) && (p.z >= vmin.z) && (p.z <= vmax.z)) return true;
        return false;
    }
    bool pip_const_z(const vector3& p) const
    {
        if ((p.x >= vmin.x) && (p.x <= vmax.x) && (p.y >= vmin.y) && (p.y <= vmax.y)) return true;
        return false;
    }
};

// Construct a bounding box around a 4x4 matrix. The translational part defines the
// center point, and the x,y,z vectors of the matrix define the extents.
inline void bbox3::set(const matrix44& m)
{
	float xExtent = n_max(n_max(n_abs(m.M11), n_abs(m.M21)), n_abs(m.M31));
	float yExtent = n_max(n_max(n_abs(m.M12), n_abs(m.M22)), n_abs(m.M32));
	float zExtent = n_max(n_max(n_abs(m.M13), n_abs(m.M23)), n_abs(m.M33));
	vector3 extent(xExtent, yExtent, zExtent);
	vector3 center = m.Translation();
	vmin = center - extent;
	vmax = center + extent;
}
//---------------------------------------------------------------------

inline void bbox3::BeginExtend()
{
	vmin.set(FLT_MAX, FLT_MAX, FLT_MAX);
	vmax.set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
}
//---------------------------------------------------------------------

inline void bbox3::BeginExtend(const vector3& InitialPoint)
{
	vmin = InitialPoint;
	vmax = InitialPoint;
}
//---------------------------------------------------------------------

inline void bbox3::EndExtend()
{
	if (vmin.x > vmax.x || vmin.y > vmax.y || vmin.z > vmax.z)
	{
		vmin = vector3::Zero;
		vmax = vector3::Zero;
	}
}
//---------------------------------------------------------------------

inline void bbox3::Extend(float x, float y, float z)
{
	if (x < vmin.x) vmin.x = x;
	if (x > vmax.x) vmax.x = x;
	if (y < vmin.y) vmin.y = y;
	if (y > vmax.y) vmax.y = y;
	if (z < vmin.z) vmin.z = z;
	if (z > vmax.z) vmax.z = z;
}
//---------------------------------------------------------------------

inline void bbox3::ExtendFast(float x, float y, float z)
{
	if (x < vmin.x) vmin.x = x;
	else if (x > vmax.x) vmax.x = x;
	if (y < vmin.y) vmin.y = y;
	else if (y > vmax.y) vmax.y = y;
	if (z < vmin.z) vmin.z = z;
	else if (z > vmax.z) vmax.z = z;
}
//---------------------------------------------------------------------

inline void bbox3::Extend(const bbox3& box)
{
	if (box.vmin.x < vmin.x) vmin.x = box.vmin.x;
	if (box.vmin.y < vmin.y) vmin.y = box.vmin.y;
	if (box.vmin.z < vmin.z) vmin.z = box.vmin.z;
	if (box.vmax.x > vmax.x) vmax.x = box.vmax.x;
	if (box.vmax.y > vmax.y) vmax.y = box.vmax.y;
	if (box.vmax.z > vmax.z) vmax.z = box.vmax.z;
}
//---------------------------------------------------------------------

inline vector3 bbox3::GetCorner(DWORD index) const
{
	n_assert_dbg(index < 8);
	switch (index)
	{
		case 0:		return vmin;
		case 1:		return vector3(vmin.x, vmax.y, vmin.z);
		case 2:		return vector3(vmax.x, vmax.y, vmin.z);
		case 3:		return vector3(vmax.x, vmin.y, vmin.z);
		case 4:		return vmax;
		case 5:		return vector3(vmin.x, vmax.y, vmax.z);
		case 6:		return vector3(vmin.x, vmin.y, vmax.z);
		default:	return vector3(vmax.x, vmin.y, vmax.z);
	}
}
//---------------------------------------------------------------------

// Get the bounding box's side planes in clip space.
inline void bbox3::get_clipplanes(const matrix44& viewProj, plane outPlanes[6]) const
{
	matrix44 inv = viewProj;
	inv.invert();
	inv.transpose();

	vector4 planes[6];
	planes[0].set(-1, 0, 0, +vmax.x);
	planes[1].set(+1, 0, 0, -vmin.x);
	planes[2].set(0, -1, 0, +vmax.y);
	planes[3].set(0, +1, 0, -vmin.y);
	planes[4].set(0, 0, -1, +vmax.z);
	planes[5].set(0, 0, +1, -vmin.z);

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
inline void bbox3::Transform(const matrix44& m)
{
	/*  ?? BUG ??
	// Extent the matrix' (x,y,z) components by our own extent vector.
	vector3 extents = extents();
	vector3 center  = center();
	matrix44 extentMatrix(
		m.M11 * extents.x, m.M12 * extents.x, m.M13 * extents.x, 0.0f,
		m.M21 * extents.y, m.M22 * extents.y, m.M23 * extents.y, 0.0f,
		m.M31 * extents.z, m.M32 * extents.z, m.M33 * extents.z, 0.0f,
		m.M41 + center.x,  m.M42 + center.y,  m.M43 + center.z,  1.0f);
	set(extentMatrix);
	*/

	//!!!For bbox3::Transform(const matrix44& m, bbox3& Out)
	//BeginExtend(m * GetCorner(0));
	//for (int i = 1; i < 8; ++i)
	//	ExtendFast(m * GetCorner(i));

	vector3 Min = m * GetCorner(0);
	vector3 Max = Min;
	for (int i = 1; i < 8; ++i)
	{
		vector3 OBBCorner = m * GetCorner(i);
		if (OBBCorner.x < Min.x) Min.x = OBBCorner.x;
		else if (OBBCorner.x > Max.x) Max.x = OBBCorner.x;
		if (OBBCorner.y < Min.y) Min.y = OBBCorner.y;
		else if (OBBCorner.y > Max.y) Max.y = OBBCorner.y;
		if (OBBCorner.z < Min.z) Min.z = OBBCorner.z;
		else if (OBBCorner.z > Max.z) Max.z = OBBCorner.z;
	}

	vmin = Min;
	vmax = Max;
}
//---------------------------------------------------------------------

// Same as Transform() but does a div-by-w on the way (useful for transforming to screen space)
inline void bbox3::transform_divw(const matrix44& m)
{
	//!!!For bbox3::transform_divw(const matrix44& m, bbox3& Out)
	//BeginExtend(m.mult_divw(GetCorner(0)));
	//for (int i = 1; i < 8; ++i)
	//	ExtendFast(m.mult_divw(GetCorner(i)));

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

	vmin = Min;
	vmax = Max;
}
//---------------------------------------------------------------------

// Check for intersection of 2 axis aligned bounding boxes in the same coordinate space
inline bool bbox3::intersects(const bbox3& box) const
{
	return	vmax.x >= box.vmin.x && vmin.x <= box.vmax.x &&
			vmax.y >= box.vmin.y && vmin.y <= box.vmax.y &&
			vmax.z >= box.vmin.z && vmin.z <= box.vmax.z;
}
//---------------------------------------------------------------------

// Check if the other box is completely contained in this bounding box
inline bool bbox3::contains(const bbox3& box) const
{
	return	vmin.x <= box.vmin.x && vmax.x >= box.vmax.x &&
			vmin.y <= box.vmin.y && vmax.y >= box.vmax.y &&
			vmin.z <= box.vmin.z && vmax.z >= box.vmax.z;
}
//---------------------------------------------------------------------

// Check if position is inside bounding box.
inline bool bbox3::contains(const vector3& v) const
{
	return	vmin.x <= v.x && vmax.x >= v.x &&
			vmin.y <= v.y && vmax.y >= v.y &&
			vmin.z <= v.z && vmax.z >= v.z;
}
//---------------------------------------------------------------------

inline EClipStatus bbox3::clipstatus(const bbox3& other) const
{
	if (contains(other)) return Inside;
	if (intersects(other)) return Clipped;
	return Outside;
}
//---------------------------------------------------------------------

// Check for intersection with a view volume defined by a view-projection matrix
inline EClipStatus bbox3::clipstatus(const matrix44& ViewProj) const
{
	int ANDFlags = 0xffff;
	int ORFlags  = 0;
	for (int i = 0; i < 8; i++)
	{
		vector4 CornerProj = ViewProj * vector4(GetCorner(i));
		int clip = 0;
		if (CornerProj.x < -CornerProj.w)       clip |= ClipLeft;
		else if (CornerProj.x > CornerProj.w)   clip |= ClipRight;
		if (CornerProj.y < -CornerProj.w)       clip |= ClipBottom;
		else if (CornerProj.y > CornerProj.w)   clip |= ClipTop;
		if (CornerProj.z < -CornerProj.w)       clip |= ClipFar;
		else if (CornerProj.z > CornerProj.w)   clip |= ClipNear;
		ANDFlags &= clip;
		ORFlags |= clip;
	}
	if (!ORFlags) return Inside;
	if (ANDFlags) return Outside;
	return Clipped;
}
//---------------------------------------------------------------------

// Create a transform matrix which transforms an unit cube to this box
inline void bbox3::ToMatrix44(matrix44& Out) const
{
    Out.set(vmax.x - vmin.x, 0.f, 0.f, 0.f,
			0.f, vmax.y - vmin.y, 0.f, 0.f,
			0.f, 0.f, vmax.z - vmin.z, 0.f,
			(vmin.x + vmax.x) * 0.5f, (vmin.y + vmax.y) * 0.5f, (vmin.z + vmax.z) * 0.5f, 1.f);
}
//---------------------------------------------------------------------

/**
    @brief Gets closest intersection with AABB.
    If the line starts inside the box,  start point is returned in ipos.
    @param line the pick ray
    @param ipos closest point of intersection if successful, trash otherwise
    @return true if an intersection occurs
*/
inline bool bbox3::intersect(const line3& line, vector3& ipos) const
{
    // Handle special case for start point inside box
    if (line.Start.x >= vmin.x && line.Start.y >= vmin.y && line.Start.z >= vmin.z &&
        line.Start.x <= vmax.x && line.Start.y <= vmax.y && line.Start.z <= vmax.z)
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
                if (isect_const_x(vmin.x,line,ipos) && pip_const_x(ipos)) return true;
                break;
            case 1:
                if (isect_const_x(vmax.x,line,ipos) && pip_const_x(ipos)) return true;
                break;
            case 2:
                if (isect_const_y(vmin.y,line,ipos) && pip_const_y(ipos)) return true;
                break;
            case 3:
                if (isect_const_y(vmax.y,line,ipos) && pip_const_y(ipos)) return true;
                break;
            case 4:
                if (isect_const_z(vmin.z,line,ipos) && pip_const_z(ipos)) return true;
                break;
            case 5:
                if (isect_const_z(vmax.z,line,ipos) && pip_const_z(ipos)) return true;
                break;
        }
    }

    return false;
}
//---------------------------------------------------------------------

inline int bbox3::line_test(float v0, float v1, float w0, float w1)
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
inline int bbox3::intersect(const bbox3& box)
{
	int and_code = 0xffff;
	int or_code = 0;
	int cx = line_test(vmin.x, vmax.x, box.vmin.x, box.vmax.x);
	and_code &= cx;
	or_code |= cx;
	int cy = line_test(vmin.y, vmax.y, box.vmin.y, box.vmax.y);
	and_code &= cy;
	or_code |= cy;
	int cz = line_test(vmin.z, vmax.z, box.vmin.z, box.vmax.z);
	and_code &= cz;
	or_code |= cz;
	if (or_code == 0) return OUTSIDE;
	if (and_code != 0) return and_code;
	if (cx && cy && cz) return CLIPS;
	return OUTSIDE;
}
//---------------------------------------------------------------------

#endif
