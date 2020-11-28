#ifndef _MATRIX44_H
#define _MATRIX44_H

#include <Math/Vector4.h>
#include <Math/Matrix33.h>
#include <memory.h>

// Matrix 4x4 class. Row-major.

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
	void set(const matrix44& m1) { memcpy(m, m1.m, sizeof(matrix44)); }
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
    /// quick multiplication, assumes that M14==M24==M34==0 and M44==1
    void mult_simple(const matrix44& m1);
	/// quick multiplication of two matrices (m1*m2) with M14==M24==M34==0 and M44==1
    void mult2_simple(const matrix44& m1, const matrix44& m2);
    /// transform vector3, projecting back into w=1
    vector3 transform_coord(const vector3& v) const;
    /// return x component
    vector3& AxisX() const { return *(vector3*)&M11; }
    /// return y component
    vector3& AxisY() const { return *(vector3*)&M21; }
    /// return z component
    vector3& AxisZ() const { return *(vector3*)&M31; }
    /// return translate component
	vector3& Translation() const { return *(vector3*)&M41; }
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
    M11 = v0.x; M12 = v0.y; M13 = v0.z; M14 = v0.w;
    M21 = v1.x; M22 = v1.y; M23 = v1.z; M24 = v1.w;
    M31 = v2.x; M32 = v2.y; M33 = v2.z; M34 = v2.w;
    M41 = v3.x; M42 = v3.y; M43 = v3.z; M44 = v3.w;
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
    M11 = _m11; M12 = _m12; M13 = _m13; M14 = _m14;
    M21 = _m21; M22 = _m22; M23 = _m23; M24 = _m24;
    M31 = _m31; M32 = _m32; M33 = _m33; M34 = _m34;
    M41 = _m41; M42 = _m42; M43 = _m43; M44 = _m44;
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
    M11=v0.x; M12=v0.y; M13=v0.z, M14=v0.w;
    M21=v1.x; M22=v1.y; M23=v1.z; M24=v1.w;
    M31=v2.x; M32=v2.y; M33=v2.z; M34=v2.w;
    M41=v3.x; M42=v3.y; M43=v3.z; M44=v3.w;
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
    M11=_m11; M12=_m12; M13=_m13; M14=_m14;
    M21=_m21; M22=_m22; M23=_m23; M24=_m24;
    M31=_m31; M32=_m32; M33=_m33; M34=_m34;
    M41=_m41; M42=_m42; M43=_m43; M44=_m44;
}

//------------------------------------------------------------------------------
/**
*/
inline void matrix44::transpose()
{
    std::swap(M12, M21);
	std::swap(M13, M31);
	std::swap(M14, M41);
	std::swap(M23, M32);
	std::swap(M24, M42);
	std::swap(M34, M43);
}

inline matrix44 matrix44::transposed() const
{
	return matrix44(
		M11, M21, M31, M41,
		M12, M22, M32, M42,
		M13, M23, M33, M43,
		M14, M24, M34, M44);
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
        (M11 * M22 - M12 * M21) * (M33 * M44 - M34 * M43)
       -(M11 * M23 - M13 * M21) * (M32 * M44 - M34 * M42)
       +(M11 * M24 - M14 * M21) * (M32 * M43 - M33 * M42)
       +(M12 * M23 - M13 * M22) * (M31 * M44 - M34 * M41)
       -(M12 * M24 - M14 * M22) * (M31 * M43 - M33 * M41)
       +(M13 * M24 - M14 * M23) * (M31 * M42 - M32 * M41);
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
        s*(M22*(M33*M44 - M34*M43) + M23*(M34*M42 - M32*M44) + M24*(M32*M43 - M33*M42)),
        s*(M32*(M13*M44 - M14*M43) + M33*(M14*M42 - M12*M44) + M34*(M12*M43 - M13*M42)),
        s*(M42*(M13*M24 - M14*M23) + M43*(M14*M22 - M12*M24) + M44*(M12*M23 - M13*M22)),
        s*(M12*(M24*M33 - M23*M34) + M13*(M22*M34 - M24*M32) + M14*(M23*M32 - M22*M33)),
        s*(M23*(M31*M44 - M34*M41) + M24*(M33*M41 - M31*M43) + M21*(M34*M43 - M33*M44)),
        s*(M33*(M11*M44 - M14*M41) + M34*(M13*M41 - M11*M43) + M31*(M14*M43 - M13*M44)),
        s*(M43*(M11*M24 - M14*M21) + M44*(M13*M21 - M11*M23) + M41*(M14*M23 - M13*M24)),
        s*(M13*(M24*M31 - M21*M34) + M14*(M21*M33 - M23*M31) + M11*(M23*M34 - M24*M33)),
        s*(M24*(M31*M42 - M32*M41) + M21*(M32*M44 - M34*M42) + M22*(M34*M41 - M31*M44)),
        s*(M34*(M11*M42 - M12*M41) + M31*(M12*M44 - M14*M42) + M32*(M14*M41 - M11*M44)),
        s*(M44*(M11*M22 - M12*M21) + M41*(M12*M24 - M14*M22) + M42*(M14*M21 - M11*M24)),
        s*(M14*(M22*M31 - M21*M32) + M11*(M24*M32 - M22*M34) + M12*(M21*M34 - M24*M31)),
        s*(M21*(M33*M42 - M32*M43) + M22*(M31*M43 - M33*M41) + M23*(M32*M41 - M31*M42)),
        s*(M31*(M13*M42 - M12*M43) + M32*(M11*M43 - M13*M41) + M33*(M12*M41 - M11*M42)),
        s*(M41*(M13*M22 - M12*M23) + M42*(M11*M23 - M13*M21) + M43*(M12*M21 - M11*M22)),
        s*(M11*(M22*M33 - M23*M32) + M12*(M23*M31 - M21*M33) + M13*(M21*M32 - M22*M31)));
}

//------------------------------------------------------------------------------
/**
    optimized determinant calculation, assumes that M14==M24==M34==0 AND M44==1
*/
inline float matrix44::det_simple() const
{
	return (M11 * M22 - M12 * M21) * (M33)
		   -(M11 * M23 - M13 * M21) * (M32)
		   +(M12 * M23 - M13 * M22) * (M31);
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
        s * ((M22 * M33) - (M23 * M32)),
        s * ((M32 * M13) - (M33 * M12)),
        s * ((M12 * M23) - (M13 * M22)),
        0.0f,
        s * ((M23 * M31) - (M21 * M33)),
        s * ((M33 * M11) - (M31 * M13)),
        s * ((M13 * M21) - (M11 * M23)),
        0.0f,
        s * ((M21 * M32) - (M22 * M31)),
        s * ((M31 * M12) - (M32 * M11)),
        s * ((M11 * M22) - (M12 * M21)),
        0.0f,
        s * (M21*(M33*M42 - M32*M43) + M22*(M31*M43 - M33*M41) + M23*(M32*M41 - M31*M42)),
        s * (M31*(M13*M42 - M12*M43) + M32*(M11*M43 - M13*M41) + M33*(M12*M41 - M11*M42)),
        s * (M41*(M13*M22 - M12*M23) + M42*(M11*M23 - M13*M21) + M43*(M12*M21 - M11*M22)),
        1.0f);
}

//------------------------------------------------------------------------------
/**
    optimized multiplication, assumes that M14==M24==M34==0 AND M44==1
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

/// quick multiplication of two matrices (m1*m2) with M14==M24==M34==0 and M44==1
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
    float d = 1.0f / (M14*v.x + M24*v.y + M34*v.z + M44);
    return vector3(
        (M11*v.x + M21*v.y + M31*v.z + M41) * d,
        (M12*v.x + M22*v.y + M32*v.z + M42) * d,
        (M13*v.x + M23*v.y + M33*v.z + M43) * d);
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
    M41 += t.x;
    M42 += t.y;
    M43 += t.z;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
matrix44::set_translation(const vector3& t)
{
    M41 = t.x;
    M42 = t.y;
    M43 = t.z;
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
    vector3 eye(M41, M42, M43);
    vector3 zaxis = eye - at;
    zaxis.norm();
    vector3 xaxis = up * zaxis;
    xaxis.norm();
    vector3 yaxis = zaxis * xaxis;
    M11 = xaxis.x;  M12 = yaxis.x;  M13 = zaxis.x;  M14 = 0.0f;
    M21 = xaxis.y;  M22 = yaxis.y;  M23 = zaxis.y;  M24 = 0.0f;
    M31 = xaxis.z;  M32 = yaxis.z;  M33 = zaxis.z;  M34 = 0.0f;
	eye *= -1.f;
    M41 = xaxis % eye; M42 = yaxis % eye; M43 = zaxis % eye; M44 = 1.0f;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
matrix44::lookatLh(const vector3& at, const vector3& up)
{
    vector3 eye(M41, M42, M43);
    vector3 zaxis = at - eye;
    zaxis.norm();
    vector3 xaxis = up * zaxis;
    xaxis.norm();
    vector3 yaxis = zaxis * xaxis;
    M11 = xaxis.x;  M12 = yaxis.x;  M13 = zaxis.x;  M14 = 0.0f;
    M21 = xaxis.y;  M22 = yaxis.y;  M23 = zaxis.y;  M24 = 0.0f;
    M31 = xaxis.z;  M32 = yaxis.z;  M33 = zaxis.z;  M34 = 0.0f;
}

//------------------------------------------------------------------------------
/**
*/
inline void matrix44::perspFovLh(float fovY, float aspect, float zn, float zf)
{
	float h = float(1.0 / tan(fovY * 0.5f));
	M11 = h / aspect;	M12 = 0.0f; M13 = 0.0f;				M14 = 0.0f;
	M21 = 0.0f;			M22 = h;    M23 = 0.0f;				M24 = 0.0f;
	M31 = 0.0f;			M32 = 0.0f; M33 = zf / (zf - zn);	M34 = 1.0f;
	M41 = 0.0f;			M42 = 0.0f; M43 = -zn * M33;		M44 = 0.0f;
}

//------------------------------------------------------------------------------
/**
*/
inline void matrix44::perspFovRh(float fovY, float aspect, float zn, float zf)
{
	float h = float(1.0 / tan(fovY * 0.5f));
	M11 = h / aspect;	M12 = 0.0f; M13 = 0.0f;             M14 = 0.0f;
	M21 = 0.0f;			M22 = h;    M23 = 0.0f;             M24 = 0.0f;
	M31 = 0.0f;			M32 = 0.0f; M33 = zf / (zn - zf);   M34 = -1.0f;
	M41 = 0.0f;			M42 = 0.0f; M43 = zn * M33;			M44 = 0.0f;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
matrix44::perspOffCenterLh(float minX, float maxX, float minY, float maxY, float zn, float zf)
{
    M11 = 2.0f * zn / (maxX - minX); M12 = 0.0f, M13 = 0.0f; M14 = 0.0f;
    M21 = 0.0f; M22 = 2.0f * zn / (maxY - minY); M23 = 0.0f; M24 = 0.0f;
    M31 = (minX + maxX) / (minX - maxX); M32 = (maxY + minY) / (minY - maxY); M33 = zf / (zf - zn); M34 = 1.0f;
    M41 = 0.0f; M42 = 0.0f; M43 = zn * zf / (zn - zf); M44 = 0.0f;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
matrix44::perspOffCenterRh(float minX, float maxX, float minY, float maxY, float zn, float zf)
{
    M11 = 2.0f * zn / (maxX - minX); M12 = 0.0f, M13 = 0.0f; M14 = 0.0f;
    M21 = 0.0f; M22 = 2.0f * zn / (maxY - minY); M23 = 0.0f; M24 = 0.0f;
    M31 = (minX + maxX) / (maxX - minX); M32 = (maxY + minY) / (maxY - minY); M33 = zf / (zn - zf); M34 = -1.0f;
    M41 = 0.0f; M42 = 0.0f; M43 = zn * zf / (zn - zf); M44 = 0.0f;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
matrix44::orthoLh(float w, float h, float zn, float zf)
{
    M11 = 2.0f / w; M12 = 0.0f;     M13 = 0.0f;             M14 = 0.0f;
    M21 = 0.0f;     M22 = 2.0f / h; M23 = 0.0f;             M24 = 0.0f;
    M31 = 0.0f;     M32 = 0.0f;     M33 = 1.0f / (zf - zn); M34 = 0.0f;
    M41 = 0.0f;     M42 = 0.0f;     M43 = zn / (zn - zf);   M44 = 1.0f;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
matrix44::orthoRh(float w, float h, float zn, float zf)
{
    M11 = 2.0f / w; M12 = 0.0f;     M13 = 0.0f;             M14 = 0.0f;
    M21 = 0.0f;     M22 = 2.0f / h; M23 = 0.0f;             M24 = 0.0f;
    M31 = 0.0f;     M32 = 0.0f;     M33 = 1.0f / (zn - zf); M34 = 0.0f;
    M41 = 0.0f;     M42 = 0.0f;     M43 = zn / (zn - zf);   M44 = 1.0f;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
matrix44::billboard(const vector3& to, const vector3& up)
{
    vector3 from(M41, M42, M43);
    vector3 z(from - to);
    z.norm();
    vector3 y(up);
    y.norm();
    vector3 x(y * z);
    z = x * y;

    M11=x.x;  M12=x.y;  M13=x.z;  M14=0.0f;
    M21=y.x;  M22=y.y;  M23=y.z;  M24=0.0f;
    M31=z.x;  M32=z.y;  M33=z.z;  M34=0.0f;
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
    rotM.M11 = ca + (1.0f - ca) * v.x * v.x;
    rotM.M12 = (1.0f - ca) * v.x * v.y - sa * v.z;
    rotM.M13 = (1.0f - ca) * v.z * v.x + sa * v.y;
    rotM.M21 = (1.0f - ca) * v.x * v.y + sa * v.z;
    rotM.M22 = ca + (1.0f - ca) * v.y * v.y;
    rotM.M23 = (1.0f - ca) * v.y * v.z - sa * v.x;
    rotM.M31 = (1.0f - ca) * v.z * v.x - sa * v.y;
    rotM.M32 = (1.0f - ca) * v.y * v.z + sa * v.x;
    rotM.M33 = ca + (1.0f - ca) * v.z * v.z;

    (*this) *= rotM;
}

//------------------------------------------------------------------------------
/**
*/
inline void matrix44::mult(const vector4& src, vector4& dst) const
{
    dst.x = M11*src.x + M21*src.y + M31*src.z + M41*src.w;
    dst.y = M12*src.x + M22*src.y + M32*src.z + M42*src.w;
    dst.z = M13*src.x + M23*src.y + M33*src.z + M43*src.w;
    dst.w = M14*src.x + M24*src.y + M34*src.z + M44*src.w;
}

//------------------------------------------------------------------------------
/**
*/
inline void matrix44::mult(const vector3& src, vector3& dst) const
{
    dst.x = M11*src.x + M21*src.y + M31*src.z + M41;
    dst.y = M12*src.x + M22*src.y + M32*src.z + M42;
    dst.z = M13*src.x + M23*src.y + M33*src.z + M43;
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
        m.M11*v.x + m.M21*v.y + m.M31*v.z + m.M41,
        m.M12*v.x + m.M22*v.y + m.M32*v.z + m.M42,
        m.M13*v.x + m.M23*v.y + m.M33*v.z + m.M43);
}

//------------------------------------------------------------------------------
/**
*/
static
inline
vector4 operator * (const matrix44& m, const vector4& v)
{
    return vector4(
        m.M11*v.x + m.M21*v.y + m.M31*v.z + m.M41*v.w,
        m.M12*v.x + m.M22*v.y + m.M32*v.z + m.M42*v.w,
        m.M13*v.x + m.M23*v.y + m.M33*v.z + m.M43*v.w,
        m.M14*v.x + m.M24*v.y + m.M34*v.z + m.M44*v.w);
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
#endif
