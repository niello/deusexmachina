#ifndef N_LIGHTNODE_H
#define N_LIGHTNODE_H
//------------------------------------------------------------------------------
/**
    @class nLightNode
    @ingroup Scene
    @brief Scene node which provides lighting information.

    NOTE: nLightNode is derived from nAbstractShaderNode, and holds most
    light parameters inside shader params. This is in order to enable
    simple animation of light parameters using existing animators.

    (C) 2003 RadonLabs GmbH
*/
#include "scene/nabstractshadernode.h"
#include "gfx2/nlight.h"

namespace Data
{
	class CBinaryReader;
}

class nLightNode: public nAbstractShaderNode
{
private:

	nLight light;

public:

	virtual bool			LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);

	virtual bool			HasLight() const { return true; }
	virtual const nLight&	ApplyLight(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& lightTransform, const vector4& shadowLightMask);
	virtual const nLight&	RenderLight(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& lightTransform);

	void					SetType(nLight::Type t) { light.SetType(t); }
	void					SetCastShadows(bool b) { light.SetCastShadows(b); }
	const nLight&			GetLight() const { return light; }
};

#endif



