#pragma once
#ifndef __DEM_L1_SCENE_LIGHT_H__
#define __DEM_L1_SCENE_LIGHT_H__

#include <Scene/SceneNodeAttr.h>

// Light is a scene node attribute describing light source properties, including type,
// color, range, shadow casting flags etc

//!!!don't forget that most of the light params are regular shader params!

class bbox3;

namespace Scene
{
struct CSPSRecord;

class CLight: public CSceneNodeAttr
{
	DeclareRTTI;
	DeclareFactory(CLight);

public:

	enum EType
	{
		Directional	= 0,
		Point		= 1,
		Spot		= 2
	};

	EType	Type;		//???Static per-instance shader var? MTE vs N3 - count of each type or type in light itself
	vector3	Color;		// Animable per-instance shaer var
	float	Intensity;	// Animable per-instance shaer var

	// ERenderFlag: ShadowCaster, DoOcclusionCulling (force disable for directionals)

	union
	{
		float Range;			// Point //???or use node tfm scale part?

		struct					// Spot //???need range too?
		{
			float ConeInner;
			float ConeOuter;
		};
	};

	//decay type(none, lin, quad), near and far attenuation
	//shadow color(or calc?)
	//???light diffuse component in reverse direction? (N2 sky node)
	//???fog intensity? decay start distance
	//???bool cast light? draw volumetric, draw ground projection

	CSPSRecord*	pSPSRecord;

	CLight(): Type(Directional), pSPSRecord(NULL), Intensity(1.f) {}

	virtual bool	LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);
	virtual void	OnRemove();
	virtual void	Update();
	void			GetBox(bbox3& OutBox) const;

	//use smallest of possible AABBs (light to model local space before testing)
	// IsBoxInRange(const bbox3& Box) { if directional yes else real test }
};

RegisterFactory(CLight);

typedef Ptr<CLight> PLight;

inline void CLight::GetBox(bbox3& OutBox) const
{
	// If local params changed, recompute AABB
	// If transform of host node changed, update global space AABB (rotate, scale)
}
//---------------------------------------------------------------------

}

#endif
