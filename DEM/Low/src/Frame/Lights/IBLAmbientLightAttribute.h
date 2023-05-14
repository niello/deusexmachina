#pragma once
#include <Frame/LightAttribute.h>
#include <Render/Texture.h>

// IBL (Image-Based Lighting) attribute contains irradiance map for ambient diffuse and
// convoluted radiance environment map for ambient specular. May be global (scene-wide)
// or local with associated volume. Blending of local cubemaps with parallax correction
// may be implemented. For implementation details read:
// https://seblagarde.wordpress.com/2012/09/29/image-based-lighting-approaches-and-parallax-corrected-cubemap/
// Irradiance map and PMREM can be generated from an environment cubemap with a
// ModifiedCubeMapGen tool in Tools/bin/cubemapgen. The tool is described in:
// https://seblagarde.wordpress.com/2012/06/10/amd-cubemapgen-for-physically-based-rendering/

namespace Render
{
	typedef Ptr<class CTexture> PTexture;
}

namespace Frame
{

class CIBLAmbientLightAttribute: public CLightAttribute
{
	FACTORY_CLASS_DECL;

protected:

	CStrID _IrradianceMapUID;
	CStrID _RadianceEnvMapUID;

public:

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual Scene::PNodeAttribute Clone() override;
	virtual bool                  GetLocalAABB(CAABB& OutBox) const override;
};

typedef Ptr<CIBLAmbientLightAttribute> PIBLAmbientLightAttribute;

}
