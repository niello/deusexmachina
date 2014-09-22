#include "Camera.h"

#include <Core/Factory.h>

namespace Scene
{
__ImplementClass(Scene::CCamera, 'CAMR', Scene::CNodeAttribute);

bool CCamera::LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader)
{
	switch (FourCC.Code)
	{
		case 'XXXX':
		{
			//DataReader.Read();
			//OK;
			FAIL;
		}
		default: FAIL;
	}
}
//---------------------------------------------------------------------

void CCamera::Update()
{
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

void CCamera::GetRay3D(float RelX, float RelY, float Length, line3& OutRay) const
{
	vector3 ScreenCoord3D((RelX - 0.5f) * 2.0f, (RelY - 0.5f) * 2.0f, 1.0f);
	vector3 LocalMousePos = (InvProj * ScreenCoord3D) * NearPlane * 1.1f;
	LocalMousePos.y = -LocalMousePos.y;
	OutRay.Start = GetInvViewMatrix() * LocalMousePos;
	OutRay.Vector = OutRay.Start - GetInvViewMatrix().Translation();
	OutRay.Vector *= (Length / OutRay.Length());
}
//---------------------------------------------------------------------

void CCamera::GetPoint2D(const vector3& Point3D, float& OutRelX, float& OutRelY) const
{
	vector4 WorldPos = View * Point3D;
	WorldPos.w = 0.f;
	WorldPos = Proj * WorldPos;
	WorldPos /= WorldPos.w;
	OutRelX = WorldPos.x * 0.5f + 0.5f;
	OutRelY = 0.5f - WorldPos.y * 0.5f;
}
//---------------------------------------------------------------------

}