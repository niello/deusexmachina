#include "Camera.h"

namespace Scene
{
ImplementRTTI(Scene::CCamera, Scene::CSceneNodeAttr);
//ImplementFactory(Scene::CCamera);

void CCamera::UpdateTransform(CScene& Scene)
{
	//???here? If local params changed, update projection matrix
	//???calc view matrix, based on InvView from node?
}
//---------------------------------------------------------------------

}