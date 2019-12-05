#include "CameraAttribute.h"

#include <Core/Factory.h>

namespace Frame
{
FACTORY_CLASS_IMPL(Frame::CCameraAttribute, 'CAMR', Scene::CNodeAttribute);

bool CCameraAttribute::LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count)
{
	/*
	for (UPTR j = 0; j < Count; ++j)
	{
		const uint32_t Code = DataReader.Read<uint32_t>();
		switch (Code)
		{
			case 'XXXX':
			{
				DataReader.Read();
				break;
			}
			default: FAIL;
		}
	}

	OK;
	*/

	return !Count;
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CCameraAttribute::Clone()
{
	PCameraAttribute ClonedAttr = n_new(CCameraAttribute);
	ClonedAttr->FOV = FOV;
	ClonedAttr->Width = Width;
	ClonedAttr->Height = Height;
	ClonedAttr->NearPlane = NearPlane;
	ClonedAttr->FarPlane = FarPlane;
	ClonedAttr->Flags.SetTo(Orthogonal, IsOrthogonal());
	return ClonedAttr.Get();
}
//---------------------------------------------------------------------

void CCameraAttribute::Update(const vector3* pCOIArray, UPTR COICount)
{
	CNodeAttribute::Update(pCOIArray, COICount);

	bool ViewOrProjChanged = false;

	if (Flags.Is(ProjDirty))
	{
		if (Flags.Is(Orthogonal)) Proj.orthoRh(Width, Height, NearPlane, FarPlane);
		else Proj.perspFovRh(FOV, Width / Height, NearPlane, FarPlane);

		// Shadow proj was calculated with:
		//nearPlane - shadowOffset, farPlane - shadowOffset, shadowOffset(0.00007f)

		//!!!avoid copying!
		InvProj = Proj;
		InvProj.invert();

		Flags.Clear(ProjDirty);
		ViewOrProjChanged = true;
	}

	if (pNode->IsWorldMatrixChanged())
	{
		pNode->GetWorldMatrix().invert_simple(View);
		ViewOrProjChanged = true;
	}

	if (ViewOrProjChanged) ViewProj = View * Proj;
}
//---------------------------------------------------------------------

void CCameraAttribute::GetRay3D(float RelX, float RelY, float Length, line3& OutRay) const
{
	vector3 ScreenCoord3D((RelX - 0.5f) * 2.0f, (RelY - 0.5f) * 2.0f, 1.0f);
	vector3 ViewLocalPos = (InvProj * ScreenCoord3D) * NearPlane * 1.1f;
	ViewLocalPos.y = -ViewLocalPos.y;
	const matrix44&	InvView = GetInvViewMatrix();
	OutRay.Start = InvView * ViewLocalPos;
	OutRay.Vector = OutRay.Start - InvView.Translation();
	OutRay.Vector *= (Length / OutRay.Length());
}
//---------------------------------------------------------------------

void CCameraAttribute::GetPoint2D(const vector3& Point3D, float& OutRelX, float& OutRelY) const
{
	vector4 WorldPos = View * Point3D;
	WorldPos.w = 0.f;
	WorldPos = Proj * WorldPos;
	WorldPos /= WorldPos.w;
	OutRelX = 0.5f + WorldPos.x * 0.5f;
	OutRelY = 0.5f - WorldPos.y * 0.5f;
}
//---------------------------------------------------------------------

}