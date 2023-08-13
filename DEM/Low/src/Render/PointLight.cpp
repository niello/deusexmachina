#include "PointLight.h"

namespace Render
{

void CPointLight::FillGPUInfo(CGPULightInfo& Out) const
{
	Out.Color = Color * Intensity;
	Out.Position = _Position;
	Out.SqInvRange = 1.f / (Range * Range);
	Out.Type = ELightType::Point;
}
//---------------------------------------------------------------------

void CPointLight::SetPosition(const vector3& Pos)
{
	_Position = Pos;
}
//---------------------------------------------------------------------

}
