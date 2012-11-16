#include "Light.h"

#include <Scene/Scene.h>

namespace Scene
{
ImplementRTTI(Scene::CLight, Scene::CSceneNodeAttr);

void CLight::UpdateTransform(CScene& Scene)
{
	if (Type == Directional) Scene.AddVisibleLight(*this);
	else
	{
		if (!pSPSRecord)
		{
			CSPSRecord* pNewRec = n_new(CSPSRecord); //!!!AddObject requires ref-to-ptr, so need to allocate -_-
			pNewRec->pAttr = this;
			GetBox(pNewRec->GlobalBox);
			pSPSRecord = Scene.SPS.AddObject(pNewRec);

			//!!!on delete attr with valid SPS handle, remove it from SPS!
		}
		else
		{
			//!!!only if local box or global tfm changed!
			GetBox(pSPSRecord->GlobalBox);
			Scene.SPS.UpdateElement(pSPSRecord);
		}
	}
}
//---------------------------------------------------------------------

}