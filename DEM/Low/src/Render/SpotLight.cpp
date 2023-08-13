#include "SpotLight.h"
#include <Math/Matrix44.h>

namespace Render
{

void CSpotLight::FillGPUInfo(CGPULightInfo& Out) const
{
	Out.Color = Color * Intensity;
	Out.Position = _Position;
	Out.InvDirection = -_Direction;
	Out.SqInvRange = 1.f / (Range * Range);
	Out.Params.x = CosHalfInner;
	Out.Params.y = CosHalfOuter;
	Out.Type = ELightType::Spot;
}
//---------------------------------------------------------------------

void CSpotLight::SetDirection(const vector3& Dir)
{
	_Direction = Dir;
	_Direction.norm();
}
//---------------------------------------------------------------------

void CSpotLight::SetPosition(const vector3& Pos)
{
	_Position = Pos;
}
//---------------------------------------------------------------------

}
