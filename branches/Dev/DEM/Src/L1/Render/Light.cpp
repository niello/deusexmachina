#include "Light.h"

#include <Render/SPS.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CLight, 'LGHT', Scene::CNodeAttribute);

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

void CLight::OnDetachFromNode()
{
	//???do it on deactivation of an attribute? even it is not detached from node
	SAFE_DELETE(pSPSRecord); // Self-removal inside a destructor
	CNodeAttribute::OnDetachFromNode();
}
//---------------------------------------------------------------------

void CLight::UpdateInSPS(CSPS& SPS)
{
	if (Type == Directional)
	{
		SAFE_DELETE(pSPSRecord); // Self-removal inside a destructor
		SPS.AlwaysVisibleLights.Add(this); //!!!shouldn't re-add itself if already added! //???use flag?
	}
	else
	{
		if (!pSPSRecord)
		{
			pSPSRecord = n_new(CSPSRecord)(*this);
			GetGlobalAABB(pSPSRecord->GlobalBox);
			SPS.AddObjectRecord(pSPSRecord);
		}
		else if (Flags.Is(WorldMatrixChanged)) //!!! || Range/Cone changed
		{
			GetGlobalAABB(pSPSRecord->GlobalBox);
			SPS.UpdateObjectRecord(pSPSRecord);
			Flags.Clear(WorldMatrixChanged);
		}
	}
}
//---------------------------------------------------------------------

//!!!GetGlobalAABB & CalcBox must be separate!
void CLight::GetGlobalAABB(CAABB& OutBox) const
{
	//!!!If local params changed, recompute AABB
	//!!!If transform of host node changed, update global space AABB (rotate, scale)
	switch (Type)
	{
		case Directional:	Sys::Error("No AABB for directional lights, must not be requested!"); return;
		case Point:			OutBox.Set(GetPosition(), vector3(Range, Range, Range)); return;
		case Spot:
		{
			//!!!can cache local box!
			float HalfFarExtent = Range * n_tan(ConeOuter / 2.f);
			OutBox.Min.set(-HalfFarExtent, -HalfFarExtent, -Range);
			OutBox.Max.set(HalfFarExtent, HalfFarExtent, 0.f);
			OutBox.Transform(pNode->GetWorldMatrix());
			return;
		}
		default:			Sys::Error("Invalid light type!");
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