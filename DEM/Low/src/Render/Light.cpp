#include "Light.h"
#include <Math/Matrix44.h>
#include <algorithm>

namespace Render
{

void CLight_OLD_DELETE::CalcLocalFrustum(matrix44& OutFrustum) const
{
	OutFrustum.perspFovRh(ConeOuter, 1.f, 0.f, Range);
}
//---------------------------------------------------------------------

float CLight_OLD_DELETE::CalcLightPriority(const vector3& ObjectPos, const vector3& LightPos, const vector3& LightInvDir) const
{
	float SqIntensity = Intensity * Intensity;
	if (Type == Light_Directional) return SqIntensity;

	float SqDistance = vector3::SqDistance(ObjectPos, LightPos);
	float Attenuation = (1.f - SqDistance * (InvRange * InvRange));

	if (Type == Light_Spot && SqDistance != 0.f)
	{
		vector3 ModelLight = ObjectPos - LightPos;
		//ModelLight /= n_sqrt(SqDistance);
		ModelLight *= Math::RSqrt(SqDistance); //!!!TEST IT!
		float CosAlpha = ModelLight.Dot(LightInvDir);
		float Falloff = (CosAlpha - CosHalfOuter) / (CosHalfInner - CosHalfOuter);
		return SqIntensity * Attenuation * std::clamp(Falloff, 0.f, 1.f);
	}

	return SqIntensity * Attenuation;
}
//---------------------------------------------------------------------

}
