#pragma once
#ifndef __DEM_L1_MATH_TFM_SRT_H__
#define __DEM_L1_MATH_TFM_SRT_H__

#include <mathlib/matrix.h>

// Transform defined by non-uniform scale, rotation quaternion and translation

namespace Math
{

class CTransformSRT
{
public:

	vector3		Translation;
	quaternion	Rotation;	// Don't assign non-unit quaternions!
	vector3		Scale;

	CTransformSRT(): Scale(1.f, 1.f, 1.f) {}
	CTransformSRT(const matrix44& Tfm) { FromMatrix(Tfm); }

	bool	FromMatrix(const matrix44& Tfm);
	void	ToMatrix(matrix44& Out) const;
};

// Default transform to this class
typedef CTransformSRT CTransform;

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

#endif
