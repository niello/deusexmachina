#pragma once
#ifndef __DEM_L1_FRAME_NODE_ATTR_AMBIENT_LIGHT_H__
#define __DEM_L1_FRAME_NODE_ATTR_AMBIENT_LIGHT_H__

#include <Scene/NodeAttribute.h>
#include <Scene/SceneNode.h>
#include <Render/Texture.h>

// IBL (Image-Based Lighting) attribute contains irradiance map for ambient diffuse and
// convoluted radiance environment map for ambient specular. May be global (scene-wide)
// or local with associated volume. Blending of local cubemaps with parallax correction
// may be implemented. See for implementation details:
// https://seblagarde.wordpress.com/2012/09/29/image-based-lighting-approaches-and-parallax-corrected-cubemap/

namespace Scene
{
	class CSPS;
	struct CSPSRecord;
}

namespace Render
{
	typedef Ptr<class CTexture> PTexture;
}

namespace Frame
{

class CNodeAttrAmbientLight: public Scene::CNodeAttribute
{
	__DeclareClass(CNodeAttrLight);

protected:

	Render::PTexture	IrradianceMap;
	Render::PTexture	RadianceEnvMap;
	Scene::CSPS*		pSPS;
	Scene::CSPSRecord*	pSPSRecord;		// NULL if oversized (global)

	virtual void	OnDetachFromScene();

public:

	CNodeAttrAmbientLight(): pSPS(NULL), pSPSRecord(NULL) {}

	virtual bool					LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader);
	virtual Scene::PNodeAttribute	Clone();
	void							UpdateInSPS(Scene::CSPS& SPS);

	bool							GetGlobalAABB(CAABB& OutBox) const;
	Render::CTexture*				GetIrradianceMap() const { return IrradianceMap.Get(); }
	Render::CTexture*				GetRadianceEnvMap() const { return RadianceEnvMap.Get(); }
};

typedef Ptr<CNodeAttrAmbientLight> PNodeAttrAmbientLight;

}

#endif
