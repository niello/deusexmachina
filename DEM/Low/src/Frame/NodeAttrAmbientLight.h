#pragma once
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
	__DeclareClass(CNodeAttrAmbientLight);

protected:

	CStrID					UIDIrradianceMap;
	CStrID					UIDRadianceEnvMap;
	Render::PTexture		IrradianceMap;
	Render::PTexture		RadianceEnvMap;
	Scene::CSPS*			pSPS = nullptr;
	Scene::CSPSRecord*		pSPSRecord = nullptr;		// nullptr if oversized (global)

	virtual void OnDetachFromScene();

public:

	virtual bool					LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count);
	virtual Scene::PNodeAttribute	Clone();
	void							UpdateInSPS(Scene::CSPS& SPS);

	bool							ValidateResources(Render::CGPUDriver* pGPU);
	bool							GetGlobalAABB(CAABB& OutBox) const;
	Render::CTexture*				GetIrradianceMap() const { return IrradianceMap.Get(); }
	Render::CTexture*				GetRadianceEnvMap() const { return RadianceEnvMap.Get(); }
};

typedef Ptr<CNodeAttrAmbientLight> PNodeAttrAmbientLight;

}
