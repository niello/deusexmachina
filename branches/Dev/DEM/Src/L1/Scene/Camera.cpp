#include "Camera.h"

namespace Scene
{
ImplementRTTI(Scene::CCamera, Scene::CSceneNodeAttr);
ImplementFactory(Scene::CCamera);

bool CCamera::LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader)
{
	switch (FourCC)
	{
		case 'DHSC': // CSHD
		{
			//!!!Flags.SetTo(ShadowCaster, DataReader.Read<bool>());!
			//DataReader.Read<bool>();
			//OK;
			FAIL;
		}
		default: FAIL;
	}
}
//---------------------------------------------------------------------

void CCamera::Update(CScene& Scene)
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

	if (GetNode()->IsWorldMatrixChanged())
	{
		//!!!avoid copying!
		View = GetNode()->GetWorldMatrix();
		View.invert_simple();
		ViewOrProjChanged = true;
	}

	if (ViewOrProjChanged) ViewProj = View * Proj;
}
//---------------------------------------------------------------------

}