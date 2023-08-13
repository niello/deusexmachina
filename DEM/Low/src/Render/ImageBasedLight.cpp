#include "ImageBasedLight.h"
#include <Render/Texture.h>

namespace Render
{

CImageBasedLight::CImageBasedLight() = default;
CImageBasedLight::~CImageBasedLight() = default;
//---------------------------------------------------------------------

void CImageBasedLight::FillGPUInfo(CGPULightInfo& Out) const
{
	// Must not be called
	NOT_IMPLEMENTED;
}
//---------------------------------------------------------------------

}
