#include "Light.h"

#include <IO/BinaryReader.h>
#include <Math/Matrix44.h>

namespace Render
{

bool CLight::LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader)
{
	switch (FourCC.Code)
	{
		case 'LGHT':
		{
			return DataReader.Read<int>((int&)Type); // To force size
		}
		case 'CSHD':
		{
			//!!!Flags.SetTo(ShadowCaster, DataReader.Read<bool>());!
			DataReader.Read<bool>();
			OK;
		}
		case 'LINT':
		{
			return DataReader.Read(Intensity);
		}
		case 'LCLR':
		{
			return DataReader.Read(Color);
		}
		case 'LRNG':
		{
			return DataReader.Read(Range);
		}
		case 'LCIN':
		{
			if (!DataReader.Read(ConeInner)) FAIL;
			SetSpotInnerAngle(n_deg2rad(ConeInner));
			OK;
		}
		case 'LCOU':
		{
			if (!DataReader.Read(ConeOuter)) FAIL;
			SetSpotOuterAngle(n_deg2rad(ConeOuter));
			OK;
		}
		default: FAIL;
	}
}
//---------------------------------------------------------------------

void CLight::CalcLocalFrustum(matrix44& OutFrustum) const
{
	OutFrustum.perspFovRh(ConeOuter, 1.f, 0.f, Range);
}
//---------------------------------------------------------------------

float CLight::CalcLightPriority(const vector3& ObjectPos, const vector3& LightPos, const vector3& LightInvDir) const
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
		return SqIntensity * Attenuation * Clamp(Falloff, 0.f, 1.f);
	}

	return SqIntensity * Attenuation;
}
//---------------------------------------------------------------------

}