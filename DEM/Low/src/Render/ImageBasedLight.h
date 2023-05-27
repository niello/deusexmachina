#pragma once
#include <Data/Ptr.h>
#include <Render/Light.h>

// Image-based light (IBL) source used for rendering. Typically used for
// ambient lighting captured from the environment. Can be global or local.
// IBL (Image-Based Lighting) contains irradiance map for ambient diffuse and
// convoluted radiance environment map for ambient specular. May be global (scene-wide)
// or local with associated volume. Blending of local cubemaps with parallax correction
// may be implemented. For implementation details read:
// https://seblagarde.wordpress.com/2012/09/29/image-based-lighting-approaches-and-parallax-corrected-cubemap/
// Irradiance map and PMREM can be generated from an environment cubemap with a
// ModifiedCubeMapGen tool in Tools/bin/cubemapgen. The tool is described in:
// https://seblagarde.wordpress.com/2012/06/10/amd-cubemapgen-for-physically-based-rendering/

namespace Render
{
using PTexture = Ptr<class CTexture>;
using PImageBasedLight = std::unique_ptr<class CImageBasedLight>;

class CImageBasedLight : public CLight
{
	RTTI_CLASS_DECL(CImageBasedLight, CLight);

protected:

public:

	//!!!TODO: protected! And add virtual function to setup renderer? Or handle different lights externally based on type? Not good, dynamic casting or type switch-case.
	Render::PTexture _IrradianceMap;
	Render::PTexture _RadianceEnvMap;

	CImageBasedLight();
	virtual ~CImageBasedLight() override;

	virtual void UpdateTransform(const matrix44& Tfm) override;
};

}
