#include "Light.h"

#include <Scene/Scene.h>
#include <Data/BinaryReader.h>

namespace Scene
{
ImplementRTTI(Scene::CLight, Scene::CSceneNodeAttr);
ImplementFactory(Scene::CLight);

bool CLight::LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader)
{
	switch (FourCC)
	{
		case 'THGL': // LGHT
		{
			return DataReader.Read<int>((int&)Type); // To force size
		}
		case 'DHSC': // CSHD
		{
			//!!!Flags.SetTo(ShadowCaster, DataReader.Read<bool>());!
			DataReader.Read<bool>();
			OK;
		}
		case 'TNIL': // LINT
		{
			return DataReader.Read(Intensity);
		}
		case 'RLCL': // LCLR
		{
			return DataReader.Read(Color);
		}
		case 'GNRL': // LRNG
		{
			return DataReader.Read(Range);
		}
		case 'NICL': // LCIN
		{
			if (!DataReader.Read(ConeInner)) FAIL;
			SetSpotInnerAngle(n_deg2rad(ConeInner));
			OK;
		}
		case 'UOCL': // LCOU
		{
			if (!DataReader.Read(ConeOuter)) FAIL;
			SetSpotOuterAngle(n_deg2rad(ConeOuter));
			OK;
		}
		default: FAIL;
	}
}
//---------------------------------------------------------------------

void CLight::OnRemove()
{
	if (pSPSRecord)
	{
		pNode->GetScene()->SPS.RemoveObject(pSPSRecord);
		n_delete(pSPSRecord);
		pSPSRecord = NULL;
	}
}
//---------------------------------------------------------------------

void CLight::Update()
{
	if (Type == Directional) pNode->GetScene()->AddVisibleLight(*this);
	else
	{
		if (!pSPSRecord)
		{
			pSPSRecord = n_new(CSPSRecord)(*this);
			GetGlobalAABB(pSPSRecord->GlobalBox);
			pNode->GetScene()->SPS.AddObject(pSPSRecord);
		}
		else if (pNode->IsWorldMatrixChanged()) //!!! || Range/Cone changed
		{
			GetGlobalAABB(pSPSRecord->GlobalBox);
			pNode->GetScene()->SPS.UpdateObject(pSPSRecord);
		}
	}
}
//---------------------------------------------------------------------

//!!!GetGlobalAABB & CalcBox must be separate!
void CLight::GetGlobalAABB(bbox3& OutBox) const
{
	//!!!If local params changed, recompute AABB
	//!!!If transform of host node changed, update global space AABB (rotate, scale)
	switch (Type)
	{
		case Directional:	n_error("No AABB for directional lights, must not be requested!"); return;
		case Point:			OutBox.set(GetPosition(), vector3(Range, Range, Range)); return;
		case Spot:
		{
			//!!!can cache local box!
			float HalfFarExtent = Range * n_tan(ConeOuter / 2.f);
			OutBox.vmin.set(-HalfFarExtent, -HalfFarExtent, -Range);
			OutBox.vmax.set(HalfFarExtent, HalfFarExtent, 0.f);
			OutBox.transform(pNode->GetWorldMatrix());
			return;
		}
		default:			n_error("Invalid light type!");
	};
}
//---------------------------------------------------------------------

void CLight::CalcFrustum(matrix44& OutFrustum)
{
	matrix44 LocalFrustum;
	LocalFrustum.perspFovRh(ConeOuter, 1.f, 0.f, Range);
	pNode->GetWorldMatrix().invert_simple(OutFrustum);
	OutFrustum *= LocalFrustum;
}
//---------------------------------------------------------------------

}