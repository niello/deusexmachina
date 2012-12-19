#include "PassOcclusion.h"

#include <scene/nsceneserver.h>

namespace Render
{

void CPassOcclusion::Render(const nArray<Scene::CRenderObject*>* pObjects, const nArray<Scene::CLight*>* pLights)
{
	nSceneServer::Instance()->DoOcclusionQuery();
}
//---------------------------------------------------------------------

}