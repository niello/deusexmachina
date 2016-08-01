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
	return ClonedAttr.GetUnsafe();
}
//---------------------------------------------------------------------

void CNodeAttrLight::OnDetachFromNode()
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

void CNodeAttrLight::UpdateInSPS(Scene::CSPS& SPS)
{
	if (Light.Type == Render::Light_Directional)
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

void CNodeAttrLight::CalcFrustum(matrix44& OutFrustum)
{
	//???cache local frustum in a CLight?
	//!!!use frustum to cull spotlights! also can cull point lights by sphere! can do on CView visibility cache collection!
	float Range = Light.GetRange();
	float ConeOuter = Light.GetSpotOuterAngle();
	matrix44 LocalFrustum;
	LocalFrustum.perspFovRh(ConeOuter, 1.f, 0.f, Range);
	pNode->GetWorldMatrix().invert_simple(OutFrustum);
	OutFrustum *= LocalFrustum;
}
//---------------------------------------------------------------------

}