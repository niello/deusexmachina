#include "Light.h"

#include <Scene/SPS.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Frame
{
__ImplementClass(Frame::CLight, 'LGHT', Scene::CNodeAttribute);

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
	if (pSPS)
	{
		if (pSPSRecord)
		{
			pSPS->RemoveRecord(pSPSRecord);
			pSPSRecord = NULL;
		}
		else pSPS->OversizedObjects.RemoveByValue(this);

		pSPS = NULL;
	}

	CNodeAttribute::OnDetachFromNode();
}
//---------------------------------------------------------------------

void CLight::UpdateInSPS(Scene::CSPS& SPS)
{
	if (Type == Directional)
	{
		if (pSPSRecord)
		{
			pSPS->RemoveRecord(pSPSRecord);
			pSPSRecord = NULL;
		}

		if (!pSPS)
		{
			pSPS = &SPS;
			SPS.OversizedObjects.Add(this);
		}
	}
	else
	{
		if (pSPS != &SPS)
		{
			if (pSPS)
			{
				if (pSPSRecord)
				{
					pSPS->RemoveRecord(pSPSRecord);
					pSPSRecord = NULL;
				}
				else pSPS->OversizedObjects.RemoveByValue(this);
			}

			pSPS = &SPS;
		}
		
		if (!pSPSRecord)
		{
			CAABB Box;
			GetGlobalAABB(Box); //???calc cached and reuse here?
			pSPSRecord = SPS.AddRecord(Box, this);
		}
		else if (Flags.Is(WorldMatrixChanged)) //!!! || Range/Cone changed
		{
			GetGlobalAABB(pSPSRecord->GlobalBox);
			SPS.UpdateRecord(pSPSRecord);
			Flags.Clear(WorldMatrixChanged);
		}
	}
}
//---------------------------------------------------------------------

//!!!GetGlobalAABB & CalcBox must be separate!
bool CLight::GetGlobalAABB(CAABB& OutBox) const
{
	//!!!If local params changed, recompute AABB
	//!!!If transform of host node changed, update global space AABB (rotate, scale)
	switch (Type)
	{
		case Directional:	FAIL;
		case Point:			OutBox.Set(GetPosition(), vector3(Range, Range, Range)); OK;
		case Spot:
		{
			//!!!can cache local box! or HalfFarExtent (1 float instead of 6)
			float HalfFarExtent = Range * n_tan(ConeOuter * 0.5f);
			OutBox.Min.set(-HalfFarExtent, -HalfFarExtent, -Range);
			OutBox.Max.set(HalfFarExtent, HalfFarExtent, 0.f);
			OutBox.Transform(pNode->GetWorldMatrix());
			OK;
		}
		default:			Sys::Error("Invalid light type!");
	};

	FAIL;
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