#include "ImageBasedLight.h"
#include <Render/Texture.h>

namespace Render
{

CImageBasedLight::CImageBasedLight() = default;
CImageBasedLight::~CImageBasedLight() = default;
//---------------------------------------------------------------------

void CImageBasedLight::UpdateTransform(const matrix44& Tfm)
{
	// if global, do nothing
	// if local, update bounds and mark them dirty
}
//---------------------------------------------------------------------

}
