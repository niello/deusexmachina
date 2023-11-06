#pragma once
#include <Math/Vector4.h>
#include <Math/Matrix33.h>
#include <memory.h>

// Matrix 4x4 class. Row-major, i.e. for affine transforms it is filled this way:
// Axx Axy Axz  0  -> X axis
// Ayx Ayy Ayz  0  -> Y axis
// Azx Azy Azz  0  -> Z axis
// Trx Try Trz  1  -> Translation
// [x] [y] [z] [w] - components of vectors
// In-memory storage order is vector by vector, i.e. Ax, Ay, Az, Tr.

class matrix44
{
public:

    float m[4][4];

	static const matrix44 Identity;
    static const matrix44 Ortho;

	matrix44() { ident(); }
    matrix44(const vector4& v0, const vector4& v1, const vector4& v2, const vector4& v3);
	matrix44(const matrix44& m1) { set(m1); }
    matrix44(float _m11, float _m12, float _m13, float _m14,
              float _m21, float _m22, float _m23, float _m24,
              float _m31, float _m32, float _m33, float _m34,
              float _m41, float _m42, float _m43, float _m44);
	matrix44(const quaternion& q) { FromQuaternion(q); }

	void		FromQuaternion(const quaternion& q);
    quaternion	ToQuaternion() const;
	matrix33	ToMatrix33() const { return matrix33(AxisX(), AxisY(), AxisZ()); }

	void set(const vector4& v0, const vector4& v1, const vector4& v2, const vector4& v3);
	void set(const matrix44& m1) { std::memcpy(m, m1.m, sizeof(matrix44)); }
    void set(float _m11, float _m12, float _m13, float _m14,
             float _m21, float _m22, float _m23, float _m24,
             float _m31, float _m32, float _m33, float _m34,
             float _m41, float _m42, float _m43, float _m44);
	void ident() { set(Identity); }
    void transpose();
	matrix44 transposed() const;
    float det() const;
    void invert();
	float det_simple() const;
	/// quick invert (if 3x3 rotation and translation)
    void invert_simple(matrix44& Out) const;
    /// quick multiplication, assumes that m[0][3]==m[1][3]==m[2][3]==0 and m[3][3]==1
    void mult_simple(const matrix44& m1);
	/// quick multiplication of two matrices (m1*m2) with m[0][3]==m[1][3]==m[2][3]==0 and m[3][3]==1
    void mult2_simple(const matrix44& m1, const matrix44& m2);
    /// transform vector3, projecting back into w=1
    vector3 transform_coord(const vector3& v) const;
    /// return x component
    vector3& AxisX() const { return *(vector3*)&m[0][0]; }
    /// return y component
    vector3& AxisY() const { return *(vector3*)&m[1][0]; }
    /// return z component
    vector3& AxisZ() const { return *(vector3*)&m[2][0]; }
    /// return translate component
	vector3& Translation() const { return *(vector3*)&m[3][0]; }
	/// extract scale from matrix
	vector3 ExtractScale() const { return vector3(AxisX().Length(), AxisY().Length(), AxisZ().Length()); }
    /// rotate around global x
    void rotate_x(const float a);
    /// rotate around global y
    void rotate_y(const float a);
    /// rotate around global z
    void rotate_z(const float a);
    /// rotate about any axis
    void rotate(const vector3& vec, float a);
    /// translate
    void translate(const vector3& t);
    /// set absolute translation
    void set_translation(const vector3& t);
    /// scale
    void scale(const vector3& s);
    /// lookat in a left-handed coordinate system
    void lookatLh(const vector3& to, const vector3& up);
    /// lookat in a right-handed coordinate system
    void lookatRh(const vector3& to, const vector3& up);
    /// create left-handed field-of-view perspective projection matrix
    void perspFovLh(float fovY, float aspect, float zn, float zf);
    /// create right-handed field-of-view perspective projection matrix
    void perspFovRh(float fovY, float aspect, float zn, float zf);
    /// create off-center left-handed perspective projection matrix
    void perspOffCenterLh(float minX, float maxX, float minY, float maxY, float zn, float zf);
    /// create off-center right-handed perspective projection matrix
    void perspOffCenterRh(float minX, float maxX, float minY, float maxY, float zn, float zf);
    /// create left-handed orthogonal projection matrix
    void orthoLh(float w, float h, float zn, float zf);
    /// create right-handed orthogonal projection matrix
    void orthoRh(float w, float h, float zn, float zf);
    /// restricted lookat
    void billboard(const vector3& to, const vector3& up);
    /// inplace matrix multiply
    void operator *= (const matrix44& m1);
	/// comparison
	bool operator ==(const matrix44& Other) const { return !memcmp(m, Other.m, sizeof(m)); }
	/// comparison
	bool operator !=(const matrix44& Other) const { return !!memcmp(m, Other.m, sizeof(m)); }
    /// multiply source vector into target vector, eliminates tmp vector
    void mult(const vector4& src, vector4& dst) const;
    /// multiply source vector into target vector, eliminates tmp vector
    void mult(const vector3& src, vector3& dst) const;
    /// multiply and divide by w
    vector3 mult_divw(const vector3& v) const;
};

DECLARE_TYPE(matrix44, 8)

//------------------------------------------------------------------------------
/**
*/
inline
matrix44::matrix44(const vector4& v0, const vector4& v1, const vector4& v2, const vector4& v3)
{
    m[0][0] = v0.x; m[0][1] = v0.y; m[0][2] = v0.z; m[0][3] = v0.w;
    m[1][0] = v1.x; m[1][1] = v1.y; m[1][2] = v1.z; m[1][3] = v1.w;
    m[2][0] = v2.x; m[2][1] = v2.y; m[2][2] = v2.z; m[2][3] = v2.w;
    m[3][0] = v3.x; m[3][1] = v3.y; m[3][2] = v3.z; m[3][3] = v3.w;
}

//------------------------------------------------------------------------------
/**
*/
inline
matrix44::matrix44(float _m11, float _m12, float _m13, float _m14,
                     float _m21, float _m22, float _m23, float _m24,
                     float _m31, float _m32, float _m33, float _m34,
                     float _m41, float _m42, float _m43, float _m44)
{
    m[0][0] = _m11; m[0][1] = _m12; m[0][2] = _m13; m[0][3] = _m14;
    m[1][0] = _m21; m[1][1] = _m22; m[1][2] = _m23; m[1][3] = _m24;
    m[2][0] = _m31; m[2][1] = _m32; m[2][2] = _m33; m[2][3] = _m34;
    m[3][0] = _m41; m[3][1] = _m42; m[3][2] = _m43; m[3][3] = _m44;
}

inline void matrix44::FromQuaternion(const quaternion& q)
{
    float wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;
    x2 = q.x + q.x; y2 = q.y + q.y; z2 = q.z + q.z;
    xx = q.x * x2;   xy = q.x * y2;   xz = q.x * z2;
    yy = q.y * y2;   yz = q.y * z2;   zz = q.z * z2;
    wx = q.w * x2;   wy = q.w * y2;   wz = q.w * z2;

    m[0][0] = 1.0f - (yy + zz);
    m[1][0] = xy - wz;
    m[2][0] = xz + wy;

    m[0][1] = xy + wz;
    m[1][1] = 1.0f - (xx + zz);
    m[2][1] = yz - wx;

    m[0][2] = xz - wy;
    m[1][2] = yz + wx;
    m[2][2] = 1.0f - (xx + yy);

    m[3][0] = m[3][1] = m[3][2] = 0.0f;
    m[0][3] = m[1][3] = m[2][3] = 0.0f;
    m[3][3] = 1.0f;
}

//------------------------------------------------------------------------------
/**
    convert orientation of 4x4 matrix into quaterion,
    4x4 matrix must not be scaled!
*/
inline
quaternion
matrix44::ToQuaternion() const
{
    float qa[4];
    float tr = m[0][0] + m[1][1] + m[2][2];
    if (tr > 0.0f)
    {
        float s = n_sqrt (tr + 1.0f);
        qa[3] = s * 0.5f;
        s = 0.5f / s;
        qa[0] = (m[1][2] - m[2][1]) * s;
        qa[1] = (m[2][0] - m[0][2]) * s;
        qa[2] = (m[0][1] - m[1][0]) * s;
    }
    else
    {
        int i, j, k, nxt[3] = {1,2,0};
        i = 0;
        if (m[1][1] > m[0][0]) i=1;
        if (m[2][2] > m[i][i]) i=2;
        j = nxt[i];
        k = nxt[j];
        float s = n_sqrt((m[i][i] - (m[j][j] + m[k][k])) + 1.0f);
        qa[i] = s * 0.5f;
        s = 0.5f / s;
        qa[3] = (m[j][k] - m[k][j])* s;
        qa[j] = (m[i][j] + m[j][i]) * s;
        qa[k] = (m[i][k] + m[k][i]) * s;
    }
    quaternion q(qa[0],qa[1],qa[2],qa[3]);
    return q;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
matrix44::set(const vector4& v0, const vector4& v1, const vector4& v2, const vector4& v3)
{
    m[0][0]=v0.x; m[0][1]=v0.y; m[0][2]=v0.z, m[0][3]=v0.w;
    m[1][0]=v1.x; m[1][1]=v1.y; m[1][2]=v1.z; m[1][3]=v1.w;
    m[2][0]=v2.x; m[2][1]=v2.y; m[2][2]=v2.z; m[2][3]=v2.w;
    m[3][0]=v3.x; m[3][1]=v3.y; m[3][2]=v3.z; m[3][3]=v3.w;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
matrix44::set(float _m11, float _m12, float _m13, float _m14,
               float _m21, float _m22, float _m23, float _m24,
               float _m31, float _m32, float _m33, float _m34,
               float _m41, float _m42, float _m43, float _m44)
{
    m[0][0]=_m11; m[0][1]=_m12; m[0][2]=_m13; m[0][3]=_m14;
    m[1][0]=_m21; m[1][1]=_m22; m[1][2]=_m23; m[1][3]=_m24;
    m[2][0]=_m31; m[2][1]=_m32; m[2][2]=_m33; m[2][3]=_m34;
    m[3][0]=_m41; m[3][1]=_m42; m[3][2]=_m43; m[3][3]=_m44;
}

//------------------------------------------------------------------------------
/**
*/
inline void matrix44::transpose()
{
    std::swap(m[0][1], m[1][0]);
	std::swap(m[0][2], m[2][0]);
	std::swap(m[0][3], m[3][0]);
	std::swap(m[1][2], m[2][1]);
	std::swap(m[1][3], m[3][1]);
	std::swap(m[2][3], m[3][2]);
}

inline matrix44 matrix44::transposed() const
{
	return matrix44(
		m[0][0], m[1][0], m[2][0], m[3][0],
		m[0][1], m[1][1], m[2][1], m[3][1],
		m[0][2], m[1][2], m[2][2], m[3][2],
		m[0][3], m[1][3], m[2][3], m[3][3]);
}
//---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
*/
inline
float
matrix44::det() const
{
    return
        (m[0][0] * m[1][1] - m[0][1] * m[1][0]) * (m[2][2] * m[3][3] - m[2][3] * m[3][2])
       -(m[0][0] * m[1][2] - m[0][2] * m[1][0]) * (m[2][1] * m[3][3] - m[2][3] * m[3][1])
       +(m[0][0] * m[1][3] - m[0][3] * m[1][0]) * (m[2][1] * m[3][2] - m[2][2] * m[3][1])
       +(m[0][1] * m[1][2] - m[0][2] * m[1][1]) * (m[2][0] * m[3][3] - m[2][3] * m[3][0])
       -(m[0][1] * m[1][3] - m[0][3] * m[1][1]) * (m[2][0] * m[3][2] - m[2][2] * m[3][0])
       +(m[0][2] * m[1][3] - m[0][3] * m[1][2]) * (m[2][0] * m[3][1] - m[2][1] * m[3][0]);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
matrix44::invert()
{
    float s = det();
    if (s == 0.0) return;
    s = 1.f / s;
    set(
        s*(m[1][1]*(m[2][2]*m[3][3] - m[2][3]*m[3][2]) + m[1][2]*(m[2][3]*m[3][1] - m[2][1]*m[3][3]) + m[1][3]*(m[2][1]*m[3][2] - m[2][2]*m[3][1])),
        s*(m[2][1]*(m[0][2]*m[3][3] - m[0][3]*m[3][2]) + m[2][2]*(m[0][3]*m[3][1] - m[0][1]*m[3][3]) + m[2][3]*(m[0][1]*m[3][2] - m[0][2]*m[3][1])),
        s*(m[3][1]*(m[0][2]*m[1][3] - m[0][3]*m[1][2]) + m[3][2]*(m[0][3]*m[1][1] - m[0][1]*m[1][3]) + m[3][3]*(m[0][1]*m[1][2] - m[0][2]*m[1][1])),
        s*(m[0][1]*(m[1][3]*m[2][2] - m[1][2]*m[2][3]) + m[0][2]*(m[1][1]*m[2][3] - m[1][3]*m[2][1]) + m[0][3]*(m[1][2]*m[2][1] - m[1][1]*m[2][2])),
        s*(m[1][2]*(m[2][0]*m[3][3] - m[2][3]*m[3][0]) + m[1][3]*(m[2][2]*m[3][0] - m[2][0]*m[3][2]) + m[1][0]*(m[2][3]*m[3][2] - m[2][2]*m[3][3])),
        s*(m[2][2]*(m[0][0]*m[3][3] - m[0][3]*m[3][0]) + m[2][3]*(m[0][2]*m[3][0] - m[0][0]*m[3][2]) + m[2][0]*(m[0][3]*m[3][2] - m[0][2]*m[3][3])),
        s*(m[3][2]*(m[0][0]*m[1][3] - m[0][3]*m[1][0]) + m[3][3]*(m[0][2]*m[1][0] - m[0][0]*m[1][2]) + m[3][0]*(m[0][3]*m[1][2] - m[0][2]*m[1][3])),
        s*(m[0][2]*(m[1][3]*m[2][0] - m[1][0]*m[2][3]) + m[0][3]*(m[1][0]*m[2][2] - m[1][2]*m[2][0]) + m[0][0]*(m[1][2]*m[2][3] - m[1][3]*m[2][2])),
        s*(m[1][3]*(m[2][0]*m[3][1] - m[2][1]*m[3][0]) + m[1][0]*(m[2][1]*m[3][3] - m[2][3]*m[3][1]) + m[1][1]*(m[2][3]*m[3][0] - m[2][0]*m[3][3])),
        s*(m[2][3]*(m[0][0]*m[3][1] - m[0][1]*m[3][0]) + m[2][0]*(m[0][1]*m[3][3] - m[0][3]*m[3][1]) + m[2][1]*(m[0][3]*m[3][0] - m[0][0]*m[3][3])),
        s*(m[3][3]*(m[0][0]*m[1][1] - m[0][1]*m[1][0]) + m[3][0]*(m[0][1]*m[1][3] - m[0][3]*m[1][1]) + m[3][1]*(m[0][3]*m[1][0] - m[0][0]*m[1][3])),
        s*(m[0][3]*(m[1][1]*m[2][0] - m[1][0]*m[2][1]) + m[0][0]*(m[1][3]*m[2][1] - m[1][1]*m[2][3]) + m[0][1]*(m[1][0]*m[2][3] - m[1][3]*m[2][0])),
        s*(m[1][0]*(m[2][2]*m[3][1] - m[2][1]*m[3][2]) + m[1][1]*(m[2][0]*m[3][2] - m[2][2]*m[3][0]) + m[1][2]*(m[2][1]*m[3][0] - m[2][0]*m[3][1])),
        s*(m[2][0]*(m[0][2]*m[3][1] - m[0][1]*m[3][2]) + m[2][1]*(m[0][0]*m[3][2] - m[0][2]*m[3][0]) + m[2][2]*(m[0][1]*m[3][0] - m[0][0]*m[3][1])),
        s*(m[3][0]*(m[0][2]*m[1][1] - m[0][1]*m[1][2]) + m[3][1]*(m[0][0]*m[1][2] - m[0][2]*m[1][0]) + m[3][2]*(m[0][1]*m[1][0] - m[0][0]*m[1][1])),
        s*(m[0][0]*(m[1][1]*m[2][2] - m[1][2]*m[2][1]) + m[0][1]*(m[1][2]*m[2][0] - m[1][0]*m[2][2]) + m[0][2]*(m[1][0]*m[2][1] - m[1][1]*m[2][0])));
}

//------------------------------------------------------------------------------
/**
    optimized determinant calculation, assumes that m[0][3]==m[1][3]==m[2][3]==0 AND m[3][3]==1
*/
inline float matrix44::det_simple() const
{
	return (m[0][0] * m[1][1] - m[0][1] * m[1][0]) * (m[2][2])
		   -(m[0][0] * m[1][2] - m[0][2] * m[1][0]) * (m[2][1])
		   +(m[0][1] * m[1][2] - m[0][2] * m[1][1]) * (m[2][0]);
}

//------------------------------------------------------------------------------
/**
    inverts a 4x4 matrix consisting of a 3x3 rotation matrix and
    a translation (eg. everything that has [0,0,0,1] as
    the rightmost column) MUCH cheaper then a real 4x4 inversion
*/
inline void matrix44::invert_simple(matrix44& Out) const
{
    float s = det_simple();
	if (s == 0.0f) return;
    s = 1.0f/s;
    Out.set(
        s * ((m[1][1] * m[2][2]) - (m[1][2] * m[2][1])),
        s * ((m[2][1] * m[0][2]) - (m[2][2] * m[0][1])),
        s * ((m[0][1] * m[1][2]) - (m[0][2] * m[1][1])),
        0.0f,
        s * ((m[1][2] * m[2][0]) - (m[1][0] * m[2][2])),
        s * ((m[2][2] * m[0][0]) - (m[2][0] * m[0][2])),
        s * ((m[0][2] * m[1][0]) - (m[0][0] * m[1][2])),
        0.0f,
        s * ((m[1][0] * m[2][1]) - (m[1][1] * m[2][0])),
        s * ((m[2][0] * m[0][1]) - (m[2][1] * m[0][0])),
        s * ((m[0][0] * m[1][1]) - (m[0][1] * m[1][0])),
        0.0f,
        s * (m[1][0]*(m[2][2]*m[3][1] - m[2][1]*m[3][2]) + m[1][1]*(m[2][0]*m[3][2] - m[2][2]*m[3][0]) + m[1][2]*(m[2][1]*m[3][0] - m[2][0]*m[3][1])),
        s * (m[2][0]*(m[0][2]*m[3][1] - m[0][1]*m[3][2]) + m[2][1]*(m[0][0]*m[3][2] - m[0][2]*m[3][0]) + m[2][2]*(m[0][1]*m[3][0] - m[0][0]*m[3][1])),
        s * (m[3][0]*(m[0][2]*m[1][1] - m[0][1]*m[1][2]) + m[3][1]*(m[0][0]*m[1][2] - m[0][2]*m[1][0]) + m[3][2]*(m[0][1]*m[1][0] - m[0][0]*m[1][1])),
        1.0f);
}

//------------------------------------------------------------------------------
/**
    optimized multiplication, assumes that m[0][3]==m[1][3]==m[2][3]==0 AND m[3][3]==1
*/
inline
void
matrix44::mult_simple(const matrix44& m1)
{
    for (UPTR i = 0; i < 4; ++i)
    {
        float mi0 = m[i][0];
        float mi1 = m[i][1];
        float mi2 = m[i][2];
        m[i][0] = mi0*m1.m[0][0] + mi1*m1.m[1][0] + mi2*m1.m[2][0];
        m[i][1] = mi0*m1.m[0][1] + mi1*m1.m[1][1] + mi2*m1.m[2][1];
        m[i][2] = mi0*m1.m[0][2] + mi1*m1.m[1][2] + mi2*m1.m[2][2];
    }
    m[3][0] += m1.m[3][0];
    m[3][1] += m1.m[3][1];
    m[3][2] += m1.m[3][2];
    m[0][3] = 0.0f;
    m[1][3] = 0.0f;
    m[2][3] = 0.0f;
    m[3][3] = 1.0f;
}

/// quick multiplication of two matrices (m1*m2) with m[0][3]==m[1][3]==m[2][3]==0 and m[3][3]==1
inline void matrix44::mult2_simple(const matrix44& m1, const matrix44& m2)
{
    for (int i = 0; i < 4; ++i)
    {
        m[i][0] = m1.m[i][0]*m2.m[0][0] + m1.m[i][1]*m2.m[1][0] + m1.m[i][2]*m2.m[2][0];
        m[i][1] = m1.m[i][0]*m2.m[0][1] + m1.m[i][1]*m2.m[1][1] + m1.m[i][2]*m2.m[2][1];
        m[i][2] = m1.m[i][0]*m2.m[0][2] + m1.m[i][1]*m2.m[1][2] + m1.m[i][2]*m2.m[2][2];
    }
    m[3][0] += m2.m[3][0];
    m[3][1] += m2.m[3][1];
    m[3][2] += m2.m[3][2];

	// Not necessary if matrix was Identity or tfm
    m[0][3] = 0.0f;
    m[1][3] = 0.0f;
    m[2][3] = 0.0f;
    m[3][3] = 1.0f;
}

//------------------------------------------------------------------------------
/**
    Transforms a vector by the matrix, projecting the result back into w=1.
*/
inline
vector3
matrix44::transform_coord(const vector3& v) const
{
    float d = 1.0f / (m[0][3]*v.x + m[1][3]*v.y + m[2][3]*v.z + m[3][3]);
    return vector3(
        (m[0][0]*v.x + m[1][0]*v.y + m[2][0]*v.z + m[3][0]) * d,
        (m[0][1]*v.x + m[1][1]*v.y + m[2][1]*v.z + m[3][1]) * d,
        (m[0][2]*v.x + m[1][2]*v.y + m[2][2]*v.z + m[3][2]) * d);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
matrix44::rotate_x(const float a)
{
    float c, s;
	n_sincos(a, s, c);
    for (int i = 0; i < 4; ++i)
	{
        float mi1 = m[i][1];
        float mi2 = m[i][2];
        m[i][1] = mi1*c + mi2*-s;
        m[i][2] = mi1*s + mi2*c;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
void
matrix44::rotate_y(const float a)
{
    float c, s;
	n_sincos(a, s, c);
    for (int i = 0; i < 4; ++i)
	{
        float mi0 = m[i][0];
        float mi2 = m[i][2];
        m[i][0] = mi0*c + mi2*s;
        m[i][2] = mi0*-s + mi2*c;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
void
matrix44::rotate_z(const float a)
{
    float c, s;
	n_sincos(a, s, c);
    for (int i = 0; i < 4; ++i)
	{
        float mi0 = m[i][0];
        float mi1 = m[i][1];
        m[i][0] = mi0*c + mi1*-s;
        m[i][1] = mi0*s + mi1*c;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
void
matrix44::translate(const vector3& t)
{
    m[3][0] += t.x;
    m[3][1] += t.y;
    m[3][2] += t.z;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
matrix44::set_translation(const vector3& t)
{
    m[3][0] = t.x;
    m[3][1] = t.y;
    m[3][2] = t.z;
};

//------------------------------------------------------------------------------
/**
*/
inline
void
matrix44::scale(const vector3& s)
{
    for (UPTR i = 0; i < 4; ++i)
    {
        m[i][0] *= s.x;
        m[i][1] *= s.y;
        m[i][2] *= s.z;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
void
matrix44::lookatRh(const vector3& at, const vector3& up)
{
    vector3 eye(m[3][0], m[3][1], m[3][2]);
    vector3 zaxis = eye - at;
    zaxis.norm();
    vector3 xaxis = up * zaxis;
    xaxis.norm();
    vector3 yaxis = zaxis * xaxis;
    m[0][0] = xaxis.x;  m[0][1] = yaxis.x;  m[0][2] = zaxis.x;  m[0][3] = 0.0f;
    m[1][0] = xaxis.y;  m[1][1] = yaxis.y;  m[1][2] = zaxis.y;  m[1][3] = 0.0f;
    m[2][0] = xaxis.z;  m[2][1] = yaxis.z;  m[2][2] = zaxis.z;  m[2][3] = 0.0f;
	eye *= -1.f;
    m[3][0] = xaxis % eye; m[3][1] = yaxis % eye; m[3][2] = zaxis % eye; m[3][3] = 1.0f;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
matrix44::lookatLh(const vector3& at, const vector3& up)
{
    vector3 eye(m[3][0], m[3][1], m[3][2]);
    vector3 zaxis = at - eye;
    zaxis.norm();
    vector3 xaxis = up * zaxis;
    xaxis.norm();
    vector3 yaxis = zaxis * xaxis;
    m[0][0] = xaxis.x;  m[0][1] = yaxis.x;  m[0][2] = zaxis.x;  m[0][3] = 0.0f;
    m[1][0] = xaxis.y;  m[1][1] = yaxis.y;  m[1][2] = zaxis.y;  m[1][3] = 0.0f;
    m[2][0] = xaxis.z;  m[2][1] = yaxis.z;  m[2][2] = zaxis.z;  m[2][3] = 0.0f;
}

//------------------------------------------------------------------------------
/**
*/
inline void matrix44::perspFovLh(float fovY, float aspect, float zn, float zf)
{
	float h = float(1.0 / tan(fovY * 0.5f));
	m[0][0] = h / aspect;	m[0][1] = 0.0f; m[0][2] = 0.0f;				m[0][3] = 0.0f;
	m[1][0] = 0.0f;			m[1][1] = h;    m[1][2] = 0.0f;				m[1][3] = 0.0f;
	m[2][0] = 0.0f;			m[2][1] = 0.0f; m[2][2] = zf / (zf - zn);	m[2][3] = 1.0f;
	m[3][0] = 0.0f;			m[3][1] = 0.0f; m[3][2] = -zn * m[2][2];	m[3][3] = 0.0f;
}

//------------------------------------------------------------------------------
/**
*/
inline void matrix44::perspFovRh(float fovY, float aspect, float zn, float zf)
{
	float h = float(1.0 / tan(fovY * 0.5f));
	m[0][0] = h / aspect;	m[0][1] = 0.0f; m[0][2] = 0.0f;             m[0][3] = 0.0f;
	m[1][0] = 0.0f;			m[1][1] = h;    m[1][2] = 0.0f;             m[1][3] = 0.0f;
	m[2][0] = 0.0f;			m[2][1] = 0.0f; m[2][2] = zf / (zn - zf);   m[2][3] = -1.0f;
	m[3][0] = 0.0f;			m[3][1] = 0.0f; m[3][2] = zn * m[2][2];		m[3][3] = 0.0f;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
matrix44::perspOffCenterLh(float minX, float maxX, float minY, float maxY, float zn, float zf)
{
    m[0][0] = 2.0f * zn / (maxX - minX); m[0][1] = 0.0f, m[0][2] = 0.0f; m[0][3] = 0.0f;
    m[1][0] = 0.0f; m[1][1] = 2.0f * zn / (maxY - minY); m[1][2] = 0.0f; m[1][3] = 0.0f;
    m[2][0] = (minX + maxX) / (minX - maxX); m[2][1] = (maxY + minY) / (minY - maxY); m[2][2] = zf / (zf - zn); m[2][3] = 1.0f;
    m[3][0] = 0.0f; m[3][1] = 0.0f; m[3][2] = zn * zf / (zn - zf); m[3][3] = 0.0f;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
matrix44::perspOffCenterRh(float minX, float maxX, float minY, float maxY, float zn, float zf)
{
    m[0][0] = 2.0f * zn / (maxX - minX); m[0][1] = 0.0f, m[0][2] = 0.0f; m[0][3] = 0.0f;
    m[1][0] = 0.0f; m[1][1] = 2.0f * zn / (maxY - minY); m[1][2] = 0.0f; m[1][3] = 0.0f;
    m[2][0] = (minX + maxX) / (maxX - minX); m[2][1] = (maxY + minY) / (maxY - minY); m[2][2] = zf / (zn - zf); m[2][3] = -1.0f;
    m[3][0] = 0.0f; m[3][1] = 0.0f; m[3][2] = zn * zf / (zn - zf); m[3][3] = 0.0f;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
matrix44::orthoLh(float w, float h, float zn, float zf)
{
    m[0][0] = 2.0f / w; m[0][1] = 0.0f;     m[0][2] = 0.0f;             m[0][3] = 0.0f;
    m[1][0] = 0.0f;     m[1][1] = 2.0f / h; m[1][2] = 0.0f;             m[1][3] = 0.0f;
    m[2][0] = 0.0f;     m[2][1] = 0.0f;     m[2][2] = 1.0f / (zf - zn); m[2][3] = 0.0f;
    m[3][0] = 0.0f;     m[3][1] = 0.0f;     m[3][2] = zn / (zn - zf);   m[3][3] = 1.0f;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
matrix44::orthoRh(float w, float h, float zn, float zf)
{
    m[0][0] = 2.0f / w; m[0][1] = 0.0f;     m[0][2] = 0.0f;             m[0][3] = 0.0f;
    m[1][0] = 0.0f;     m[1][1] = 2.0f / h; m[1][2] = 0.0f;             m[1][3] = 0.0f;
    m[2][0] = 0.0f;     m[2][1] = 0.0f;     m[2][2] = 1.0f / (zn - zf); m[2][3] = 0.0f;
    m[3][0] = 0.0f;     m[3][1] = 0.0f;     m[3][2] = zn / (zn - zf);   m[3][3] = 1.0f;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
matrix44::billboard(const vector3& to, const vector3& up)
{
    vector3 from(m[3][0], m[3][1], m[3][2]);
    vector3 z(from - to);
    z.norm();
    vector3 y(up);
    y.norm();
    vector3 x(y * z);
    z = x * y;

    m[0][0]=x.x;  m[0][1]=x.y;  m[0][2]=x.z;  m[0][3]=0.0f;
    m[1][0]=y.x;  m[1][1]=y.y;  m[1][2]=y.z;  m[1][3]=0.0f;
    m[2][0]=z.x;  m[2][1]=z.y;  m[2][2]=z.z;  m[2][3]=0.0f;
}

//------------------------------------------------------------------------------
/**
*/
inline void matrix44::rotate(const vector3& vec, float a)
{
    vector3 v(vec);
    v.norm();
    float sa = (float) n_sin(a);
    float ca = (float) n_cos(a);

    matrix44 rotM;
    rotM.m[0][0] = ca + (1.0f - ca) * v.x * v.x;
    rotM.m[0][1] = (1.0f - ca) * v.x * v.y - sa * v.z;
    rotM.m[0][2] = (1.0f - ca) * v.z * v.x + sa * v.y;
    rotM.m[1][0] = (1.0f - ca) * v.x * v.y + sa * v.z;
    rotM.m[1][1] = ca + (1.0f - ca) * v.y * v.y;
    rotM.m[1][2] = (1.0f - ca) * v.y * v.z - sa * v.x;
    rotM.m[2][0] = (1.0f - ca) * v.z * v.x - sa * v.y;
    rotM.m[2][1] = (1.0f - ca) * v.y * v.z + sa * v.x;
    rotM.m[2][2] = ca + (1.0f - ca) * v.z * v.z;

    (*this) *= rotM;
}

//------------------------------------------------------------------------------
/**
*/
inline void matrix44::mult(const vector4& src, vector4& dst) const
{
    dst.x = m[0][0]*src.x + m[1][0]*src.y + m[2][0]*src.z + m[3][0]*src.w;
    dst.y = m[0][1]*src.x + m[1][1]*src.y + m[2][1]*src.z + m[3][1]*src.w;
    dst.z = m[0][2]*src.x + m[1][2]*src.y + m[2][2]*src.z + m[3][2]*src.w;
    dst.w = m[0][3]*src.x + m[1][3]*src.y + m[2][3]*src.z + m[3][3]*src.w;
}

//------------------------------------------------------------------------------
/**
*/
inline void matrix44::mult(const vector3& src, vector3& dst) const
{
    dst.x = m[0][0]*src.x + m[1][0]*src.y + m[2][0]*src.z + m[3][0];
    dst.y = m[0][1]*src.x + m[1][1]*src.y + m[2][1]*src.z + m[3][1];
    dst.z = m[0][2]*src.x + m[1][2]*src.y + m[2][2]*src.z + m[3][2];
}

//------------------------------------------------------------------------------
/**
*/
static
inline
matrix44 operator * (const matrix44& m0, const matrix44& m1)
{
    matrix44 m2(
        m0.m[0][0]*m1.m[0][0] + m0.m[0][1]*m1.m[1][0] + m0.m[0][2]*m1.m[2][0] + m0.m[0][3]*m1.m[3][0],
        m0.m[0][0]*m1.m[0][1] + m0.m[0][1]*m1.m[1][1] + m0.m[0][2]*m1.m[2][1] + m0.m[0][3]*m1.m[3][1],
        m0.m[0][0]*m1.m[0][2] + m0.m[0][1]*m1.m[1][2] + m0.m[0][2]*m1.m[2][2] + m0.m[0][3]*m1.m[3][2],
        m0.m[0][0]*m1.m[0][3] + m0.m[0][1]*m1.m[1][3] + m0.m[0][2]*m1.m[2][3] + m0.m[0][3]*m1.m[3][3],

        m0.m[1][0]*m1.m[0][0] + m0.m[1][1]*m1.m[1][0] + m0.m[1][2]*m1.m[2][0] + m0.m[1][3]*m1.m[3][0],
        m0.m[1][0]*m1.m[0][1] + m0.m[1][1]*m1.m[1][1] + m0.m[1][2]*m1.m[2][1] + m0.m[1][3]*m1.m[3][1],
        m0.m[1][0]*m1.m[0][2] + m0.m[1][1]*m1.m[1][2] + m0.m[1][2]*m1.m[2][2] + m0.m[1][3]*m1.m[3][2],
        m0.m[1][0]*m1.m[0][3] + m0.m[1][1]*m1.m[1][3] + m0.m[1][2]*m1.m[2][3] + m0.m[1][3]*m1.m[3][3],

        m0.m[2][0]*m1.m[0][0] + m0.m[2][1]*m1.m[1][0] + m0.m[2][2]*m1.m[2][0] + m0.m[2][3]*m1.m[3][0],
        m0.m[2][0]*m1.m[0][1] + m0.m[2][1]*m1.m[1][1] + m0.m[2][2]*m1.m[2][1] + m0.m[2][3]*m1.m[3][1],
        m0.m[2][0]*m1.m[0][2] + m0.m[2][1]*m1.m[1][2] + m0.m[2][2]*m1.m[2][2] + m0.m[2][3]*m1.m[3][2],
        m0.m[2][0]*m1.m[0][3] + m0.m[2][1]*m1.m[1][3] + m0.m[2][2]*m1.m[2][3] + m0.m[2][3]*m1.m[3][3],

        m0.m[3][0]*m1.m[0][0] + m0.m[3][1]*m1.m[1][0] + m0.m[3][2]*m1.m[2][0] + m0.m[3][3]*m1.m[3][0],
        m0.m[3][0]*m1.m[0][1] + m0.m[3][1]*m1.m[1][1] + m0.m[3][2]*m1.m[2][1] + m0.m[3][3]*m1.m[3][1],
        m0.m[3][0]*m1.m[0][2] + m0.m[3][1]*m1.m[1][2] + m0.m[3][2]*m1.m[2][2] + m0.m[3][3]*m1.m[3][2],
        m0.m[3][0]*m1.m[0][3] + m0.m[3][1]*m1.m[1][3] + m0.m[3][2]*m1.m[2][3] + m0.m[3][3]*m1.m[3][3]);
    return m2;
}

//------------------------------------------------------------------------------
/**
*/
static
inline
vector3 operator * (const matrix44& m, const vector3& v)
{
    return vector3(
        m.m[0][0]*v.x + m.m[1][0]*v.y + m.m[2][0]*v.z + m.m[3][0],
        m.m[0][1]*v.x + m.m[1][1]*v.y + m.m[2][1]*v.z + m.m[3][1],
        m.m[0][2]*v.x + m.m[1][2]*v.y + m.m[2][2]*v.z + m.m[3][2]);
}

//------------------------------------------------------------------------------
/**
*/
static
inline
vector4 operator * (const matrix44& m, const vector4& v)
{
    return vector4(
        m.m[0][0]*v.x + m.m[1][0]*v.y + m.m[2][0]*v.z + m.m[3][0]*v.w,
        m.m[0][1]*v.x + m.m[1][1]*v.y + m.m[2][1]*v.z + m.m[3][1]*v.w,
        m.m[0][2]*v.x + m.m[1][2]*v.y + m.m[2][2]*v.z + m.m[3][2]*v.w,
        m.m[0][3]*v.x + m.m[1][3]*v.y + m.m[2][3]*v.z + m.m[3][3]*v.w);
};

//------------------------------------------------------------------------------
/**
*/
inline
void
matrix44::operator *= (const matrix44& m1)
{
    for (UPTR i = 0; i < 4; ++i)
    {
        float mi0 = m[i][0];
        float mi1 = m[i][1];
        float mi2 = m[i][2];
        float mi3 = m[i][3];
        m[i][0] = mi0*m1.m[0][0] + mi1*m1.m[1][0] + mi2*m1.m[2][0] + mi3*m1.m[3][0];
        m[i][1] = mi0*m1.m[0][1] + mi1*m1.m[1][1] + mi2*m1.m[2][1] + mi3*m1.m[3][1];
        m[i][2] = mi0*m1.m[0][2] + mi1*m1.m[1][2] + mi2*m1.m[2][2] + mi3*m1.m[3][2];
        m[i][3] = mi0*m1.m[0][3] + mi1*m1.m[1][3] + mi2*m1.m[2][3] + mi3*m1.m[3][3];
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
vector3
matrix44::mult_divw(const vector3& v) const
{
    vector4 v4(v.x, v.y, v.z, 1.0f);
    v4 = *this * v4;
    return vector3(v4.x / v4.w, v4.y / v4.w, v4.z / v4.w);
}

//------------------------------------------------------------------------------
