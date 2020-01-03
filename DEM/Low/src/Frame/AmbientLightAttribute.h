#pragma once
#include <Scene/NodeAttribute.h>
#include <Scene/SceneNode.h>
#include <Render/Texture.h>

// IBL (Image-Based Lighting) attribute contains irradiance map for ambient diffuse and
// convoluted radiance environment map for ambient specular. May be global (scene-wide)
// or local with associated volume. Blending of local cubemaps with parallax correction
// may be implemented. For implementation details read:
// https://seblagarde.wordpress.com/2012/09/29/image-based-lighting-approaches-and-parallax-corrected-cubemap/
// Irradiance map and PMREM can be generated from an environment cubemap with a
// ModifiedCubeMapGen tool in Tools/bin/cubemapgen. The tool is described in:
// https://seblagarde.wordpress.com/2012/06/10/amd-cubemapgen-for-physically-based-rendering/

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
class CGraphicsResourceManager;

class CAmbientLightAttribute: public Scene::CNodeAttribute
{
	FACTORY_CLASS_DECL;

protected:

	CStrID             IrradianceMapUID;
	CStrID             RadianceEnvMapUID;
	Render::PTexture   IrradianceMap;
	Render::PTexture   RadianceEnvMap;
	Scene::CSPS*       pSPS = nullptr;
	Scene::CSPSRecord* pSPSRecord = nullptr; // remains nullptr if oversized (global)

	virtual void OnDetachFromScene();

public:

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual Scene::PNodeAttribute Clone() override;
	void                          UpdateInSPS(Scene::CSPS& SPS);

	void                          SetIrradianceMap(CStrID UID) { IrradianceMapUID = UID; IrradianceMap = nullptr; }
	void                          SetRadianceEnvMap(CStrID UID) { RadianceEnvMapUID = UID; RadianceEnvMap = nullptr; }

	bool                          ValidateGPUResources(CGraphicsResourceManager& ResMgr);
	bool                          GetGlobalAABB(CAABB& OutBox) const;
	Render::CTexture*             GetIrradianceMap() const { return IrradianceMap.Get(); }
	Render::CTexture*             GetRadianceEnvMap() const { return RadianceEnvMap.Get(); }
};

typedef Ptr<CAmbientLightAttribute> PAmbientLightAttribute;

}
