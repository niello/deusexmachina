#include "ImageBasedLight.h"
#include <Render/Texture.h>

namespace Render
{

CImageBasedLight::CImageBasedLight()
{
	GPUData.Type = ELightType::IBL;
}
//---------------------------------------------------------------------

CImageBasedLight::~CImageBasedLight() = default;
//---------------------------------------------------------------------

}
