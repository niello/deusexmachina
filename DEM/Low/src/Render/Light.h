#pragma once
#include <Core/RTTIBaseClass.h>
#include <Data/Flags.h>
#include <Math/Vector3.h>
#include <System/System.h>

// A base class for light source instances used for rendering. This is a GPU-friendly implementation
// created from CLightAttribute found in a 3D scene. Subclass for different source types.

namespace Render
{
typedef std::unique_ptr<class CLight> PLight;

class CLight : public Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL(Render::CLight, Core::CRTTIBaseClass);

protected:

	// ...

public:

	bool IsVisible = true;
	U32  BoundsVersion = 0;

	//???shadow casting flag etc here?
};

}

class matrix44;

namespace Render
{

enum ELightType
{
	Light_Directional	= 0,
	Light_Point			= 1,
	Light_Spot			= 2
};

class CLight_OLD_DELETE
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

	CLight_OLD_DELETE();

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

inline CLight_OLD_DELETE::CLight_OLD_DELETE():
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

inline void CLight_OLD_DELETE::SetRange(float NewRange)
{
	n_assert(NewRange > 0.f);
	Range = NewRange;
	InvRange = 1.f / Range;
}
//---------------------------------------------------------------------

inline void CLight_OLD_DELETE::SetSpotInnerAngle(float NewAngle)
{
	n_assert(NewAngle > 0.f && NewAngle < PI);
	n_assert_dbg(NewAngle < ConeOuter);
	ConeInner = NewAngle;
	CosHalfInner = n_cos(ConeInner * 0.5f);
}
//---------------------------------------------------------------------

inline void CLight_OLD_DELETE::SetSpotOuterAngle(float NewAngle)
{
	n_assert(NewAngle > 0.f && NewAngle < PI);
	ConeOuter = NewAngle;
	CosHalfOuter = n_cos(ConeOuter * 0.5f);
}
//---------------------------------------------------------------------

}
