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

}