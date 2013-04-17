#include "Camera.h"

namespace Scene
{
ImplementRTTI(Scene::CCamera, Scene::CSceneNodeAttr);
ImplementFactory(Scene::CCamera);

bool CCamera::LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader)
{
	switch (FourCC)
	{
		case 'XXXX': // XXXX
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

void CCamera::GetRay3D(float RelX, float RelY, float Length, line3& OutRay)
{
	vector3 ScreenCoord3D((RelX - 0.5f) * 2.0f, (RelY - 0.5f) * 2.0f, 1.0f);
	vector3 LocalMousePos = (InvProj * ScreenCoord3D) * NearPlane * 1.1f;
	LocalMousePos.y = -LocalMousePos.y;
	OutRay.b = pNode->GetWorldMatrix() * LocalMousePos;
	OutRay.m = OutRay.b - pNode->GetWorldMatrix().pos_component();
	OutRay.m *= (Length / OutRay.m.len());
}
//---------------------------------------------------------------------

}