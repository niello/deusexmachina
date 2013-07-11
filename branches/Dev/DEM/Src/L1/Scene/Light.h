#pragma once
#ifndef __DEM_L1_SCENE_LIGHT_H__
#define __DEM_L1_SCENE_LIGHT_H__

#include <Scene/NodeAttribute.h>
#include <Scene/SceneNode.h>

// Light is a scene node attribute describing light source properties, including type,
// color, range, shadow casting flags etc

//!!!don't forget that most of the light params are regular shader params!

class bbox3;

namespace Scene
{
struct CSPSRecord;

class CLight: public CNodeAttribute
{
	__DeclareClass(CLight);

protected:

	// Point & Spot
	float		Range;
	float		InvRange;

	// Spot
	float		ConeInner;	// In radians, full angle (not half), Theta
	float		ConeOuter;	// In radians, full angle (not half), Phi
	float		CosHalfInner;
	float		CosHalfOuter;

public:

	enum EType
	{
		Directional	= 0,
		Point		= 1,
		Spot		= 2
	};

	// ERenderFlag: ShadowCaster, DoOcclusionCulling (force disable for directionals)

	EType		Type;
	vector3		Color;		//???What with alpha color?
	float		Intensity;

	//shadow color(or calc?)
	//???light diffuse component in reverse direction? (N2 sky node)
	//???fog intensity?
	//???bool cast light? draw volumetric, draw ground projection

	CSPSRecord*	pSPSRecord;

	CLight();

	virtual bool	LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader);
	virtual void	OnDetachFromNode();
	virtual void	Update();

	void			CalcFrustum(matrix44& OutFrustum);
	void			GetGlobalAABB(bbox3& OutBox) const;

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
