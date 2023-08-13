#include "DirectionalLight.h"
#include <Math/Matrix44.h>

namespace Render
{

void CDirectionalLight::FillGPUInfo(CGPULightInfo& Out) const
{
	Out.Color = Color * Intensity;
	Out.InvDirection = -_Direction;
	Out.Type = ELightType::Directional;
}
//---------------------------------------------------------------------

void CDirectionalLight::SetDirection(const vector3& Dir)
{
	_Direction = Dir;
	_Direction.norm();
}
//---------------------------------------------------------------------

}
