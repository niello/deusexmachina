#pragma once
#ifndef __DEM_L1_FRAME_LIGHT_H__
#define __DEM_L1_FRAME_LIGHT_H__

#include <Scene/NodeAttribute.h>
#include <Scene/SceneNode.h>

// Light is a scene node attribute describing light source properties, including type,
// color, range, shadow casting flags etc

class CAABB;

namespace Scene
{
	class CSPS;
	struct CSPSRecord;
}

namespace Frame
{

class CLight: public Scene::CNodeAttribute
{
	__DeclareClass(CLight);

protected:

	enum // extends Scene::CNodeAttribute enum
	{
		// Active
		// WorldMatrixChanged
		AddedAsAlwaysVisible	= 0x04,	// To avoid searching in SPS AlwaysVisible array at each UpdateInSPS() call
		DoOcclusionCulling		= 0x08,	// Don't use for directional lights
		CastShadow				= 0x10
	};

	// Point & Spot
	float		Range;
	float		InvRange;

	// Spot
	float		ConeInner;		// In radians, full angle (not half), Theta
	float		ConeOuter;		// In radians, full angle (not half), Phi
	float		CosHalfInner;
	float		CosHalfOuter;

	union
	{
		Scene::CSPSRecord*	pSPSRecord;	// If added to a spatial structure
		Scene::CSPS*		pSPS;		// If added as oversized (directional)
	};

public:

	enum EType
	{
		Directional	= 0,
		Point		= 1,
		Spot		= 2
	};

	EType		Type;
	vector3		Color;		//???What with alpha color? place intensity there?
	float		Intensity;

	CLight();

	virtual bool	LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader);
	virtual void	OnDetachFromNode();

	void			UpdateInSPS(Scene::CSPS& SPS);
	void			CalcFrustum(matrix44& OutFrustum);
	bool			GetGlobalAABB(CAABB& OutBox) const;

	void			SetRange(float NewRange);
	void			SetSpotInnerAngle(float NewAngle);
	void			SetSpotOuterAngle(float NewAngle);
	const vector3&	GetPosition() const { return pNode->GetWorldMatrix().Translation(); }
	vector3			GetDirection() const { return -pNode->GetWorldMatrix().AxisZ(); }
	const vector3&	GetReverseDirection() const { return pNode->GetWorldMatrix().AxisZ(); }
	float			GetRange() const { return Range; }
	float			GetInvRange() const { return InvRange; }
	float			GetSpotInnerAngle() const { return ConeInner; }
	float			GetSpotOuterAngle() const { return ConeOuter; }
	float			GetCosHalfTheta() const { return CosHalfInner; }
	float			GetCosHalfPhi() const { return CosHalfOuter; }
};

typedef Ptr<CLight> PLight;

inline CLight::CLight():
	Type(Directional),
	pSPSRecord(NULL),
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

#endif
