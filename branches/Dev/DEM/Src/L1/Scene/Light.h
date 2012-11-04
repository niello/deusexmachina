#pragma once
#ifndef __DEM_L1_SCENE_LIGHT_H__
#define __DEM_L1_SCENE_LIGHT_H__

#include <Scene/SceneNodeAttr.h>

// Light is a scene node attribute describing light source properties, including type,
// color, range, shadow casting flags etc

//!!!don't forget that most of the light params are regular shader params!

namespace Scene
{

class CLight: public CSceneNodeAttr
{
public:

	enum EType
	{
		Point		= 0,
		Directional	= 1,
		Spot		= 2
	};

	EType	Type;
	vector3	Color;
	float	Intensity;
	bool	CastShadows; //???to flags?

	union
	{
		float Range;			// Point //???or use node tfm scale part?

		struct					// Spot
		{
			float ConeInner;
			float ConeOuter;
		};
	};

	//decay type, near and far attenuation, inner cone, outer cone,
	//shadow color(or calc?)
	//???fog intensity? decay start distance
	//???bool cast light? volumetric, ground projection
};

}

#endif
