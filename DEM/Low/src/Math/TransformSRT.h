#pragma once
#include <Math/Matrix44.h>

// Transform defined by non-uniform scale, rotation quaternion and translation

namespace Math
{

class CTransformSRT
{
public:

	static const CTransformSRT Identity;

	vector3		Translation;
	quaternion	Rotation;	// Don't assign non-unit quaternions!
	vector3		Scale;

	constexpr CTransformSRT(): Scale(1.f, 1.f, 1.f) {}
	CTransformSRT(const vector3& S, const quaternion& R, const vector3& T) : Scale(S), Rotation(R), Translation(T) {}
	CTransformSRT(const matrix44& Tfm) { FromMatrix(Tfm); }

	bool FromMatrix(const matrix44& Tfm);
	void ToMatrix(matrix44& Out) const;
};

inline void CTransformSRT::ToMatrix(matrix44& Out) const
{
	//Rotation.normalize();
	Out.set(Scale.x,	0.f,		0.f,		0.f,
			0.f,		Scale.y,	0.f,		0.f,
			0.f,		0.f,		Scale.z,	0.f,
			0.f,		0.f,		0.f,		1.f);
	Out.mult_simple(matrix44(Rotation)); //???is there better way?
	Out.set_translation(Translation);
}
//---------------------------------------------------------------------

}
