// geometry.cpp	-- tu@tulrich.com Copyright 1998 Thatcher Ulrich

// Implementations for the basic geometric types.


#include <math.h>
#include <float.h>
#include "geometry.h"


const vec3	vec3::zero( 0, 0, 0 );
const vec3	vec3::x_axis( 1, 0, 0 );
const vec3	vec3::y_axis( 0, 1, 0 );
const vec3	vec3::z_axis( 0, 0, 1 );


vec3	vec3::operator+(const vec3& v) const
// Adds two vec3s.  Creates a temporary for the return value.
{
	vec3	result;
	result.x = x + v.x;
	result.y = y + v.y;
	result.z = z + v.z;
	return result;
}


vec3	vec3::operator-(const vec3& v) const
// Subtracts two vectors.  Creates a temporary for the return value.
{
	vec3	result;
	result.x = x - v.x;
	result.y = y - v.y;
	result.z = z - v.z;
	return result;
}


vec3	vec3::operator-() const
// Returns the negative of *this.  Creates a temporary for the return value.
{
	vec3	result;
	result.x = -x;
	result.y = -y;
	result.z = -z;
	return result;
}


#ifndef INLINE_VEC3


float	vec3::operator*(const vec3& v) const
// Dot product.
{
	float	result;
	result = x * v.x;
	result += y * v.y;
	result += z * v.z;
	return result;
}


vec3&	vec3::operator+=(const vec3& v)
// Adds a vec3 to *this.
{
	x += v.x;
	y += v.y;
	z += v.z;
	return *this;
}


vec3&	vec3::operator-=(const vec3& v)
// Subtracts a vec3 from *this.
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
	return *this;
}


#endif // INLINE_VEC3


vec3	vec3::operator*(float f) const
// Scale a vec3 by a scalar.  Creates a temporary for the return value.
{
	vec3	result;
	result.x = x * f;
	result.y = y * f;
	result.z = z * f;
	return result;
}


vec3	vec3::cross(const vec3& v) const
// Cross product.  Creates a temporary for the return value.
{
	vec3	result;
	result.x = y * v.z - z * v.y;
	result.y = z * v.x - x * v.z;
	result.z = x * v.y - y * v.x;
	return result;
}


vec3&	vec3::normalize()
// Scales the vec3 to unit length.  Preserves its direction.
{
	float	f = magnitude();
	if (f < 0.0000001) {
		// Punt.
		x = 1;
		y = 0;
		z = 0;
	} else {
		this->operator/=(f);
	}
	return *this;
}


vec3&	vec3::operator*=(float f)
// Scales *this by the given scalar.
{
	x *= f;
	y *= f;
	z *= f;
	return *this;
}


float	vec3::magnitude() const
// Returns the length of *this.
{
	return sqrtf(sqrmag());
}


float	vec3::sqrmag() const
// Returns the square of the length of *this.
{
	return x * x + y * y * z * z;
}


void	vec3::read(SDL_RWops* in)
// Read our values from the given stream.
{
	x = ReadFloat32(in);
	y = ReadFloat32(in);
	z = ReadFloat32(in);
}


void	vec3::write(SDL_RWops* out)
// Write our contents to the given stream.
{
	WriteFloat32(out, x);
	WriteFloat32(out, y);
	WriteFloat32(out, z);
}


bool	vec3::checknan() const
// Returns true if any component is nan.
{
	if (x != x || y != y || z != z) {
//	if (fabs(x) > 10000000 || fabs(y) > 10000000 || fabs(z) > 10000000) {
		return true;//xxxxxxx
	}
//	if (_isnan(m[0]) || _isnan(m[1]) || _isnan(m[2])) {
//		return true;
//	}
	else return false;
}


// class matrix


void	matrix::Identity()
// Sets *this to be an identity matrix.
{
	SetColumn(0, vec3::x_axis);
	SetColumn(1, vec3::y_axis);
	SetColumn(2, vec3::z_axis);
	SetColumn(3, vec3::zero);
}


void	matrix::View(const vec3& ViewNormal, const vec3& ViewUp, const vec3& ViewLocation)
// Turns *this into a view matrix, given the direction the camera is
// looking (ViewNormal) and the camera's up vec3 (ViewUp), and its
// location (ViewLocation) (all vec3s in world-coordinates).  The
// resulting matrix will transform points from world coordinates to view
// coordinates, which is a right-handed system with the x axis pointing
// left, the y axis pointing up, and the z axis pointing into the scene.
{
	vec3	ViewX = ViewUp.cross(ViewNormal);

	// Construct the view-to-world matrix.
	Orient(ViewX, ViewUp, ViewLocation);

	// Turn it around, to make it world-to-view.
	Invert();
}


void	matrix::Orient(const vec3& Direction, const vec3& Up, const vec3& Location)
// Turns *this into a transformation matrix, that transforms vec3s
// from object coordinates to world coordinates, given the object's Direction, Up,
// and Location in world coordinates.
{
	vec3	ZAxis = Direction.cross(Up);

	SetColumn(0, Direction);
	SetColumn(1, Up);
	SetColumn(2, ZAxis);
	SetColumn(3, Location);
}


vec3	matrix::operator*(const vec3& v) const
// Applies *this to the given vec3, and returns the transformed vec3.
{
	vec3	result;
	Apply(&result, v);
	return result;
}


matrix	matrix::operator*(const matrix& a) const
// Composes the two matrices, returns the product.  Creates a temporary
// for the return value.
{
	matrix result;

	Compose(&result, *this, a);

	return result;
}


void	matrix::Compose(matrix* dest, const matrix& left, const matrix& right)
// Multiplies left * right, and puts the result in *dest.
{
	left.ApplyRotation(&const_cast<vec3&>(dest->GetColumn(0)), right.GetColumn(0));
	left.ApplyRotation(&const_cast<vec3&>(dest->GetColumn(1)), right.GetColumn(1));
	left.ApplyRotation(&const_cast<vec3&>(dest->GetColumn(2)), right.GetColumn(2));
	left.Apply(&const_cast<vec3&>(dest->GetColumn(3)), right.GetColumn(3));
}


matrix&	matrix::operator*=(float f)
// Scalar multiply of a matrix.
{
	int	i;
	for (i = 0; i < 4; i++) m[i] *= f;
	return *this;
}


matrix&	matrix::operator+=(const matrix& mat)
// Memberwise matrix addition.
{
	int	i;
	for (i = 0; i < 4; i++) m[i] += mat.m[i];
	return *this;
}


void	matrix::Invert()
// Inverts *this.  Uses transpose property of rotation matrices.
{
	InvertRotation();

	// Compute the translation part of the inverted matrix, by applying
	// the inverse rotation to the original translation.
	vec3	TransPrime;
	ApplyRotation(&TransPrime, GetColumn(3));

	SetColumn(3, -TransPrime);	// Could optimize the negation by doing it in-place.
}


void	matrix::InvertRotation()
// Inverts the rotation part of *this.  Ignores the translation.
// Uses the transpose property of rotation matrices.
{
	float	f;

	// Swap elements across the diagonal.
	f = m[1].get(0);
	m[1].set(0, m[0].get(1));
	m[0].set(1, f);

	f = m[2].get(0);
	m[2].set(0, m[0].get(2));
	m[0].set(2, f);

	f = m[2].get(1);
	m[2].set(1, m[1].get(2));
	m[1].set(2, f);
}


void	matrix::NormalizeRotation()
// Normalizes the rotation part of the matrix.
{
	m[0].normalize();
	m[1] = m[2].cross(m[0]);
	m[1].normalize();
	m[2] = m[0].cross(m[1]);
}


void	matrix::Apply(vec3* result, const vec3& v) const
// Applies v to *this, and puts the transformed result in *result.
{
	// Do the rotation.
	ApplyRotation(result, v);
	// Do the translation.
	*result += m[3];
}


void	matrix::ApplyRotation(vec3* result, const vec3& v) const
// Applies the rotation portion of *this, and puts the transformed result in *result.
{
	result->set(0, m[0].get(0) * v.get(0) + m[1].get(0) * v.get(1) + m[2].get(0) * v.get(2));
	result->set(1, m[0].get(1) * v.get(0) + m[1].get(1) * v.get(1) + m[2].get(1) * v.get(2));
	result->set(2, m[0].get(2) * v.get(0) + m[1].get(2) * v.get(1) + m[2].get(2) * v.get(2));
}


void	matrix::ApplyInverse(vec3* result, const vec3& v) const
// Applies v to the inverse of *this, and puts the transformed result in *result.
{
	ApplyInverseRotation(result, v - m[3]);
}


void	matrix::ApplyInverseRotation(vec3* result, const vec3& v) const
// Applies v to the inverse rotation part of *this, and puts the result in *result.
{
	result->set(0, m[0] * v);
	result->set(1, m[1] * v);
	result->set(2, m[2] * v);
}


void	matrix::Translate(const vec3& v)
// Composes a translation on the right of *this.
{
	vec3	newtrans;
	Apply(&newtrans, v);
	SetColumn(3, newtrans);
}


void	matrix::SetOrientation(const quaternion& q)
// Sets the rotation part of the matrix to the values which correspond to the given
// quaternion orientation.
{
	float	S = q.GetS();
	const vec3&	V = q.GetV();
	
	m[0].set(0, 1 - 2 * V.get_y() * V.get_y() - 2 * V.get_z() * V.get_z());
	m[0].set(1, 2 * V.get_x() * V.get_y() + 2 * S * V.get_z());
	m[0].set(2, 2 * V.get_x() * V.get_z() - 2 * S * V.get_y());

	m[1].set(0, 2 * V.get_x() * V.get_y() - 2 * S * V.get_z());
	m[1].set(1, 1 - 2 * V.get_x() * V.get_x() - 2 * V.get_z() * V.get_z());
	m[1].set(2, 2 * V.get_y() * V.get_z() + 2 * S * V.get_x());

	m[2].set(0, 2 * V.get_x() * V.get_z() + 2 * S * V.get_y());
	m[2].set(1, 2 * V.get_y() * V.get_z() - 2 * S * V.get_x());
	m[2].set(2, 1 - 2 * V.get_x() * V.get_x() - 2 * V.get_y() * V.get_y());
}


quaternion	matrix::GetOrientation() const
// Converts the rotation part of *this into a quaternion, and returns it.
{
	// Code adapted from Baraff, "Rigid Body Simulation", from SIGGRAPH 95 course notes for Physically Based Modeling.
	quaternion	q;
	float	tr, s;

	tr = m[0].get(0) + m[1].get(1) + m[2].get(2);	// trace

	if (tr >= 0) {
		s = sqrtf(tr + 1);
		q.SetS(0.5f * s);
		s = 0.5f / s;
		q.SetV(vec3(m[1].get(2) - m[2].get(1), m[2].get(0) - m[0].get(2), m[0].get(1) - m[1].get(0)) * s);
	} else {
		int	i = 0;

		if (m[1].get(1) > m[0].get(0)) {
			i = 1;
		}
		if (m[2].get(2) > m[i].get(i)) {
			i = 2;
		}

		float	qr, qi, qj, qk;
		switch (i) {
		default:
		case 0:
			s = sqrtf((m[0].get(0) - (m[1].get(1) + m[2].get(2))) + 1);
			qi = 0.5f * s;
			s = 0.5f / s;
			qj = (m[1].get(0) + m[0].get(1)) * s;
			qk = (m[0].get(2) + m[2].get(0)) * s;
			qr = (m[1].get(2) - m[2].get(1)) * s;
			break;

		case 1:
			s = sqrtf((m[1].get(1) - (m[2].get(2) + m[0].get(0))) + 1);
			qj = 0.5f * s;
			s = 0.5f / s;
			qk = (m[2].get(1) + m[1].get(2)) * s;
			qi = (m[1].get(0) + m[0].get(1)) * s;
			qr = (m[2].get(0) - m[0].get(2)) * s;
			break;

		case 2:
			s = sqrtf((m[2].get(2) - (m[0].get(0) + m[1].get(1))) + 1);
			qk = 0.5f * s;
			s = 0.5f / s;
			qi = (m[0].get(2) + m[2].get(0)) * s;
			qj = (m[2].get(1) + m[1].get(2)) * s;
			qr = (m[0].get(1) - m[1].get(0)) * s;
			break;
		}
		q.SetS(qr);
		q.SetV(vec3(qi, qj, qk));
	}

	return q;
}


//
// class quaternion
//


quaternion::quaternion(const vec3& Axis, float Angle)
// Constructs the quaternion defined by the rotation about the given axis of the given angle (in radians).
{
	S = cosf(Angle / 2);
	V = Axis;
	V *= sinf(Angle / 2);
}


quaternion	quaternion::operator*(const quaternion& q) const
// Multiplication of two quaternions.  Returns a new quaternion
// result.
{
	return quaternion(S * q.S - V * q.V, q.V * S + V * q.S + V.cross(q.V));
}


quaternion&	quaternion::normalize()
// Ensures that the quaternion has magnitude 1.
{
	float	mag = sqrtf(S * S + V * V);
	if (mag > 0.0000001) {
		float	inv = 1.0f / mag;
		S *= inv;
		V *= inv;
	} else {
		// Quaternion is messed up.  Turn it into a null rotation.
		S = 1;
		V = vec3::zero;
	}

	return *this;
}


quaternion&	quaternion::operator*=(const quaternion& q)
// In-place quaternion multiplication.
{
	*this = *this * q;	// I don't think we can avoid making temporaries.

	return *this;
}


void	quaternion::ApplyRotation(vec3* result, const vec3& v)
// Rotates the given vec3 v by the rotation represented by this quaternion.
// Stores the result in the given result vec3.
{
	quaternion	q(*this * quaternion(0, v) * quaternion(S, -V));	// There's definitely a shortcut here.  Deal with later.

	*result = q.V;
}


quaternion	quaternion::lerp(const quaternion& q, float f) const
// Does a spherical linear interpolation between *this and q, using f as
// the blend factor.  f == 0 --> result == *this, f == 1 --> result == q.
{
	quaternion	result;

	float	f0, f1;

	float	cos_omega = V * q.V + S * q.S;
	quaternion	qtemp(q);

	// Adjust signs if necessary.
	if (cos_omega < 0) {
		cos_omega = -cos_omega;
		qtemp.V = -qtemp.V;
		qtemp.S = -qtemp.S;
	}

	if (cos_omega < 0.99) {
		// Do the spherical interp.
		float	omega = acosf(cos_omega);
		float	sin_omega = sinf(omega);
		f0 = sinf((1 - f) * omega) / sin_omega;
		f1 = sinf(f * omega) / sin_omega;
	} else {
		// Quaternions are close; just do straight lerp and avoid division by near-zero.
		f0 = 1 - f;
		f1 = f;
	}
	
	result.S = S * f0 + qtemp.S * f1;
	result.V = V * f0 + qtemp.V * f1;
	result.normalize();

	return result;
}



#ifdef NOT
QuatSlerp(QUAT * from, QUAT * to, float t, QUAT * res)
      {
        float           to1[4];
        double        omega, cosom, sinom, scale0, scale1;

        // calc cosine
        cosom = from->x * to->x + from->y * to->y + from->z * to->z
			       + from->w * to->w;

        // adjust signs (if necessary)
        if ( cosom <0.0 ){ cosom = -cosom; to1[0] = - to->x;
		to1[1] = - to->y;
		to1[2] = - to->z;
		to1[3] = - to->w;
        } else  {
		to1[0] = to->x;
		to1[1] = to->y;
		to1[2] = to->z;
		to1[3] = to->w;
        }

        // calculate coefficients

       if ( (1.0 - cosom) > DELTA ) {
                // standard case (slerp)
                omega = acos(cosom);
                sinom = sin(omega);
                scale0 = sin((1.0 - t) * omega) / sinom;
                scale1 = sin(t * omega) / sinom;

        } else {        
    // "from" and "to" quaternions are very close 
	    //  ... so we can do a linear interpolation
                scale0 = 1.0 - t;
                scale1 = t;
        }
	// calculate final values
	res->x = scale0 * from->x + scale1 * to1[0];
	res->y = scale0 * from->y + scale1 * to1[1];
	res->z = scale0 * from->z + scale1 * to1[2];
	res->w = scale0 * from->w + scale1 * to1[3];
}
 
 
#endif // NOT


#ifdef NOT
void	bitmap32::ProcessForColorKeyZero()
// Examine alpha channel, and set pixels that have alpha==0 to color 0.
// Pixels that have alpha > 0.5, but have color 0, should be tweaked slightly
// so they don't get knocked out when blitting.
{
	uint32	Key32 = 0;
	uint32*	p = GetData();
	int	pixels = GetHeight() * GetWidth();
	for (int i = 0; i < pixels; i++, p++) {
		if ((*p >> 24) >= 128) {
			// Alpha >= 0.5.  Make sure color isn't equal to color key.
			if ((*p & 0x00FFFFFF) == Key32) {
				*p ^= 8;	// Twiddle a low blue bit.
			}
		} else {
			// Set pixel's color equal to color key.
			*p = (*p & 0xFF000000) | Key32;
		}
	}		
}
#endif // NOT


namespace Geometry {
;


vec3	Rotate(float Angle, const vec3& Axis, const vec3& Point)
// Rotates the given point through the given angle (in radians) about the given
// axis.
{
	quaternion	q(cosf(Angle/2), Axis * sinf(Angle/2));

	vec3	result;
	q.ApplyRotation(&result, Point);

	return result;
}


};
