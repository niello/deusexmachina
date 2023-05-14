#pragma once
#include <Data/Flags.h>
#include <Math/Vector3.h>
#include <System/System.h>

// Light describes light source properties, including type, color, range, shadow casting flags etc

class matrix44;

namespace IO
{
	class CBinaryReader;
}

namespace Render
{

enum ELightType
{
	Light_Directional	= 0,
	Light_Point			= 1,
	Light_Spot			= 2
};

class CLight
{
protected:

	enum
	{
		DoOcclusionCulling		= 0x04,	// Don't use for directional lights
		CastShadow				= 0x08
	};

	Data::CFlags		Flags;

	// Point & Spot
	float				Range;
	float				InvRange;

	// Spot
	float				ConeInner;		// In radians, full angle (not half), Theta
	float				ConeOuter;		// In radians, full angle (not half), Phi
	float				CosHalfInner;
	float				CosHalfOuter;

public:

	ELightType			Type;
	vector3				Color;		//???What with alpha color? place intensity there?
	float				Intensity;

	CLight();

	void			CalcLocalFrustum(matrix44& OutFrustum) const; // Spot only
	float			CalcLightPriority(const vector3& ObjectPos, const vector3& LightPos, const vector3& LightInvDir) const;

	void			SetRange(float NewRange);
	void			SetSpotInnerAngle(float NewAngle);
	void			SetSpotOuterAngle(float NewAngle);
	float			GetRange() const { return Range; }
	float			GetInvRange() const { return InvRange; }
	float			GetSpotInnerAngle() const { return ConeInner; }
	float			GetSpotOuterAngle() const { return ConeOuter; }
	float			GetCosHalfTheta() const { return CosHalfInner; }
	float			GetCosHalfPhi() const { return CosHalfOuter; }
};

inline CLight::CLight():
	Type(Light_Directional),
	Color(1.f, 1.f, 1.f),
	Intensity(0.5f),
	Range(1.f),
	InvRange(1.f),
	ConeInner(PI / 3.f),
	ConeOuter(PI / 2.f)
{
	CosHalfInner = n_cos(ConeInner * 0.5f);
	CosHalfOuter = n_cos(ConeOuter * 0.5f);
}
//---------------------------------------------------------------------

inline void CLight::SetRange(float NewRange)
{
	n_assert(NewRange > 0.f);
	Range = NewRange;
	InvRange = 1.f / Range;
}
//---------------------------------------------------------------------

inline void CLight::SetSpotInnerAngle(float NewAngle)
{
	n_assert(NewAngle > 0.f && NewAngle < PI);
	n_assert_dbg(NewAngle < ConeOuter);
	ConeInner = NewAngle;
	CosHalfInner = n_cos(ConeInner * 0.5f);
}
//---------------------------------------------------------------------

inline void CLight::SetSpotOuterAngle(float NewAngle)
{
	n_assert(NewAngle > 0.f && NewAngle < PI);
	ConeOuter = NewAngle;
	CosHalfOuter = n_cos(ConeOuter * 0.5f);
}
//---------------------------------------------------------------------

}
