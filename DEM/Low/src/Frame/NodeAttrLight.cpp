#include "NodeAttrLight.h"

#include <Scene/SPS.h>
#include <Render/Light.h>
#include <Core/Factory.h>

namespace Frame
{
__ImplementClass(Frame::CNodeAttrLight, 'NALT', Scene::CNodeAttribute);

bool CNodeAttrLight::LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader)
{
	return Light.LoadDataBlock(FourCC, DataReader);
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CNodeAttrLight::Clone()
{
	PNodeAttrLight ClonedAttr = n_new(CNodeAttrLight);
	ClonedAttr->Light = Light;
	return ClonedAttr.Get();
}
//---------------------------------------------------------------------

void CNodeAttrLight::OnDetachFromScene()
{
	//???do it on deactivation of an attribute? even it is not detached from node
	if (pSPS)
	{
		if (pSPSRecord)
		{
			pSPS->RemoveRecord(pSPSRecord);
			pSPSRecord = nullptr;
		}
		else pSPS->OversizedObjects.RemoveByValue(this);

		pSPS = nullptr;
	}

	CNodeAttribute::OnDetachFromScene();
}
//---------------------------------------------------------------------

void CNodeAttrLight::UpdateInSPS(Scene::CSPS& SPS)
{
	if (Light.Type == Render::Light_Directional)
	{
		if (pSPSRecord)
		{
			pSPS->RemoveRecord(pSPSRecord);
			pSPSRecord = nullptr;
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
					pSPSRecord = nullptr;
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
			Flags.Clear(WorldMatrixChanged);
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
bool CNodeAttrLight::GetGlobalAABB(CAABB& OutBox) const
{
	//!!!If local params changed, recompute AABB
	//!!!If transform of host node changed, update global space AABB (rotate, scale)
	switch (Light.Type)
	{
		case Render::Light_Directional:	FAIL;
		case Render::Light_Point:
		{
			float Range = Light.GetRange();
			OutBox.Set(GetPosition(), vector3(Range, Range, Range));
			OK;
		}
		case Render::Light_Spot:
		{
			//!!!can cache local box! or HalfFarExtent (1 float instead of 6)
			float Range = Light.GetRange();
			float ConeOuter = Light.GetSpotOuterAngle();
			float HalfFarExtent = Range * n_tan(ConeOuter * 0.5f);
			OutBox.Min.set(-HalfFarExtent, -HalfFarExtent, -Range);
			OutBox.Max.set(HalfFarExtent, HalfFarExtent, 0.f);
			OutBox.Transform(pNode->GetWorldMatrix());
			OK;
		}
		default:	Sys::Error("Invalid light type!");
	};

	FAIL;
}
//---------------------------------------------------------------------

void CNodeAttrLight::CalcFrustum(matrix44& OutFrustum) const
{
	matrix44 LocalFrustum;
	Light.CalcLocalFrustum(LocalFrustum);
	pNode->GetWorldMatrix().invert_simple(OutFrustum);
	OutFrustum *= LocalFrustum;
}
//---------------------------------------------------------------------

}