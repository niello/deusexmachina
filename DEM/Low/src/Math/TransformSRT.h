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
	CTransformSRT(const matrix44& Tfm) { FromMatrix(Tfm); }

	bool	FromMatrix(const matrix44& Tfm);
	void	ToMatrix(matrix44& Out) const;

	void Accumulate(const CTransformSRT& Other)
	{
		// Blend with shortest arc, based on a 4D dot product sign
		if (Rotation.x * Other.Rotation.x + Rotation.y * Other.Rotation.y + Rotation.z * Other.Rotation.z + Rotation.w * Other.Rotation.w < 0.f)
			Rotation -= Other.Rotation;
		else
			Rotation += Other.Rotation;

		Translation += Other.Translation;
		Scale += Other.Scale;
	}

	void operator *=(float Weight)
	{
		Translation *= Weight;
		Rotation.x *= Weight;
		Rotation.y *= Weight;
		Rotation.z *= Weight;
		Rotation.w *= Weight;
		Scale *= Weight;
	}
};

// Make this class a default transform representation
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
