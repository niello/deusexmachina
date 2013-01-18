#include "Light.h"

#include <Scene/Scene.h>
#include <Data/BinaryReader.h>

//!!!OLD!
#include <gfx2/nshaderstate.h>

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
		default: FAIL;
	}
}
//---------------------------------------------------------------------

void CLight::OnRemove()
{
	if (pSPSRecord)
	{
		pNode->GetScene()->SPS.RemoveElement(pSPSRecord);
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
			CSPSRecord NewRec(*this);
			GetBox(NewRec.GlobalBox);
			pSPSRecord = pNode->GetScene()->SPS.AddObject(NewRec);
		}
		else if (pNode->IsWorldMatrixChanged()) //!!! || Range/Cone changed
		{
			GetBox(pSPSRecord->GlobalBox);
			pNode->GetScene()->SPS.UpdateElement(pSPSRecord);
		}
	}
}
//---------------------------------------------------------------------

//!!!GetBox & CalcBox must be separate!
void CLight::GetBox(bbox3& OutBox) const
{
	n_assert2(Type != Directional && Type != Spot, "IMPLEMENT SPOTLIGHT AABB!!!");
	// If local params changed, recompute AABB
	// If transform of host node changed, update global space AABB (rotate, scale)
	OutBox.set(GetPosition(), vector3(Range, Range, Range));
}
//---------------------------------------------------------------------

void CLight::CalcFrustum(matrix44& OutFrustum)
{
	matrix44 LocalFrustum;
	LocalFrustum.perspFovRh(ConeOuter, 1.f, 0.f, Range);
	OutFrustum = pNode->GetWorldMatrix();
	OutFrustum.invert_simple();
	OutFrustum *= LocalFrustum;
}
//---------------------------------------------------------------------

}